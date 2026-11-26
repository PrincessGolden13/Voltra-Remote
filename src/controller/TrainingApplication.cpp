#include "TrainingApplication.h"
#include "diagnostics/EventLog.h"
#include "protocol/VoltraGattProfile.h"
#include <algorithm>
namespace voltra {
namespace {
constexpr uint64_t discoveryIntervalMs=5000;
// Three scan intervals tolerate missed radio packets; never machine readiness.
constexpr uint64_t discoveryGraceMs=15000;
constexpr size_t maximumDiscoveredDevices=64;
// Device-screen discovery policy, not control/readiness freshness. Probe only
// the latest scan's advertisers, one endpoint per window. Existing short radio
// retries fit inside that window; discovery adds no retry layer of its own.
constexpr uint64_t topologyProbeIntervalMs=15000,topologyPresentationMs=30000;
// Recheck a confirmed standalone endpoint at the next eligible scan. It can
// become paired while Devices remains open. Failed probes keep the slower rate.
constexpr uint64_t standaloneTopologyProbeIntervalMs=discoveryIntervalMs;
constexpr uint64_t topologyConnectDeadlineMs=4500,topologyReadDeadlineMs=3000;
constexpr uint64_t motionFreshMs=500;
// NimBLE wraps Bluetooth HCI error 0x13 (remote user terminated) in its 0x200
// host-error range. Only an ended, established link can use this indication.
constexpr int peerTerminatedLinkCode=0x200+0x13;
constexpr uint64_t restPauseBarDelayMs=250; // User-requested visual threshold.
// The ordinary read-only path visits each candidate at most once. Allow BLE
// connection/bootstrap time, then six seconds for independent role evidence.
constexpr uint64_t selectionDeadlineMs=30000,selectionReadDeadlineMs=6000;
// Allow the deliberate known-controller recovery its radio budget, previous
// link teardown, fresh GATT setup and read-only Twin verification.
constexpr uint64_t twinRecoveryDeadlineMs=75000;
const char* restPauseLimitError="RP pause must be shorter than Auto Unload";
std::string connectingStatus(const std::string& name,const std::string& peer={}) {
    const std::string device=name.empty() ? "VOLTRA" : name;
    if(peer.empty()) return "Connecting to "+device+"...";
    if(device=="VOLTRA" && peer=="VOLTRA") return "Connecting to Twin...";
    return "Connecting to "+device+" and "+peer+"...";
}
}
TrainingApplication::TrainingApplication(BleTransport& t, VoltraController& c, ControlSession& s, DeviceMemory* m, std::vector<DeviceEntry> knownDevices, bool automaticSelection)
    : transport_(t), controller_(c), session_(s), memory_(m), knownDevices_(std::move(knownDevices)), automaticSelection_(automaticSelection) {
    transport_.setScanServices({VoltraGattProfile::service});
    if (memory_) recent_ = memory_->load();
    // Address type belongs to the current radio observation, not the logical
    // device. Keep the existing saved key and remap old aliases/relationships.
    const auto normalize=[&](std::vector<DeviceEntry>& entries) {
        std::vector<DeviceEntry> unique;
        for(auto d:entries) {
            d.id=logicalDeviceId(d.id);
            if(!d.twinPeerId.empty()) d.twinPeerId=logicalDeviceId(d.twinPeerId);
            if(!d.twinSelectionId.empty()) d.twinSelectionId=logicalDeviceId(d.twinSelectionId);
            if(d.twinPeerId==d.id || (!d.twinSelectionId.empty() && d.twinSelectionId!=d.id && d.twinSelectionId!=d.twinPeerId)) {
                d.twinPeerId.clear();d.twinPeer.clear();d.twinSelectionId.clear();
            }
            const auto existing=std::find_if(unique.begin(),unique.end(),[&](const auto& entry){return entry.id==d.id;});
            if(existing==unique.end()) unique.push_back(std::move(d));
            else {
                if(existing->name.empty() || (existing->name=="VOLTRA" && !d.name.empty())) existing->name=d.name;
                if(existing->twinPeerId.empty() && !d.twinPeerId.empty()) {
                    existing->twinPeerId=d.twinPeerId;existing->twinPeer=d.twinPeer;existing->twinSelectionId=d.twinSelectionId;
                }
            }
        }
        entries=std::move(unique);
    };
    normalize(recent_);normalize(knownDevices_);
    for (auto& d : recent_) {
        d.online=d.twinFresh=d.twinPeerAvailable=false;
        if(d.twinPeerId.empty()) {d.twinPeer.clear();d.twinSelectionId.clear();continue;}
        const auto peer=std::find_if(recent_.begin(),recent_.end(),[&](const auto& entry){return entry.id==d.twinPeerId;});
        const bool valid=d.twinPeerId!=d.id && peer!=recent_.end() && peer->twinPeerId==d.id &&
            peer->twinSelectionId==d.twinSelectionId && (d.twinSelectionId==d.id || d.twinSelectionId==d.twinPeerId);
        if(valid) twinLinks_[d.id]={peer->id,peer->name,0,false,d.twinSelectionId,true};
        else {d.twinPeer.clear();d.twinPeerId.clear();d.twinSelectionId.clear();}
    }
}
bool TrainingApplication::sameDeviceIdentity(const DeviceId& a,const DeviceId& b) const {
    if(a==b) return true;
    const auto address=transport_.deviceAddress(a);
    return !address.empty() && address==transport_.deviceAddress(b);
}
DeviceId TrainingApplication::logicalDeviceId(const DeviceId& id) const {
    if(id.empty() || transport_.deviceAddress(id).empty()) return id;
    if(!selected_.empty() && sameDeviceIdentity(id,selected_)) return selected_;
    for(const auto& d:recent_) if(sameDeviceIdentity(id,d.id)) return d.id;
    for(const auto& d:knownDevices_) if(sameDeviceIdentity(id,d.id)) return d.id;
    for(const auto& seen:seenDevices_) if(sameDeviceIdentity(id,seen.first)) return seen.first;
    return id;
}
bool TrainingApplication::remembered(const DeviceId& id) const {
    return std::any_of(recent_.begin(), recent_.end(), [&](const auto& d) { return sameDeviceIdentity(d.id,id); });
}
DeviceId TrainingApplication::selectionDevice(const DeviceId& id) const {
    const auto logical=logicalDeviceId(id);
    const auto link=twinLinks_.find(logical);
    if(link!=twinLinks_.end() && !link->second.selection.empty()) return logicalDeviceId(link->second.selection);
    const auto device=std::find_if(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==logical;});
    return device!=devices_.end() && !device->twinSelectionId.empty() ? logicalDeviceId(device->twinSelectionId) : logical;
}
DeviceId TrainingApplication::reachableSelectionEndpoint(const DeviceId& id) const {
    const auto preferred=selectionDevice(id);
    const auto visible=[&](const DeviceId& candidate) {
        return now_>=latestAdvertisedAt_ && now_-latestAdvertisedAt_<=discoveryGraceMs &&
            std::find(latestAdvertised_.begin(),latestAdvertised_.end(),candidate)!=latestAdvertised_.end();
    };
    if(visible(preferred)) return preferred;
    const auto link=twinLinks_.find(preferred);
    if(link!=twinLinks_.end() && visible(link->second.peer)) return link->second.peer;
    // A saved pair is navigation only. Starting at its advertising member does
    // not authorize it as the primary; normal selection rechecks live evidence.
    return preferred;
}
void TrainingApplication::setTopologyDiscoveryEnabled(bool enabled) {
    topologyDiscoveryEnabled_=enabled;
    if(!enabled && topologyProbeActive_) stopTopologyProbe();
}
void TrainingApplication::stopTopologyProbe() {
    if(!topologyProbeActive_) return;
    controller_.disconnect();
    topologyProbeActive_=false;topologyProbeStartedAt_=0;topologyReadAt_.reset();
    selected_.clear();name_.clear();
    refreshTwinDevices();
}
void TrainingApplication::startTopologyProbe() {
    if(!topologyDiscoveryEnabled_ || !topologyProbeDue_ || maintenance_ || discoveryPaused_ ||
       topologyProbeActive_ || selectionPhase_!=SelectionPhase::Idle || radioInUse() || scanPending_ ||
       !selected_.empty()) return;
    topologyProbeDue_=false;
    if(now_<latestAdvertisedAt_ || now_-latestAdvertisedAt_>discoveryIntervalMs) return;
    const DeviceEntry* candidate=nullptr;
    uint64_t oldest=0;
    for(const auto& id:latestAdvertised_) {
        const auto found=std::find_if(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==id;});
        if(found==devices_.end()) continue;
        const auto previous=topologyProbes_.find(id);
        if(previous!=topologyProbes_.end()) {
            const auto interval=previous->second.confirmedAlone ? standaloneTopologyProbeIntervalMs : topologyProbeIntervalMs;
            if(now_>=previous->second.at && now_-previous->second.at<interval) continue;
        }
        const uint64_t at=previous==topologyProbes_.end() ? 0 : previous->second.at;
        if(!candidate || at<oldest) {candidate=&*found;oldest=at;}
    }
    if(!candidate || !session_.setReadOnlyProbe(true)) return;
    markTopologyProbed(candidate->id);
    selected_=candidate->id;name_=candidate->name;
    topologyProbeActive_=true;topologyProbeStartedAt_=now_;topologyReadAt_.reset();
    diagnostic("Device topology probe started");
    if(!controller_.connect(selected_)) {stopTopologyProbe();return;}
    session_.setSelectedAddress(transport_.deviceAddress(selected_));
}
void TrainingApplication::markTopologyProbed(const DeviceId& id) {
    if(topologyProbes_.size()>=maximumDiscoveredDevices && !topologyProbes_.count(id)) {
        const auto oldestEntry=std::min_element(topologyProbes_.begin(),topologyProbes_.end(),
            [](const auto& a,const auto& b){return a.second.at<b.second.at;});
        topologyProbes_.erase(oldestEntry);
    }
    topologyProbes_[id]={now_,false};
}
void TrainingApplication::tickTopologyProbe() {
    if(!topologyDiscoveryEnabled_ || maintenance_ || discoveryPaused_ || !session_.readOnlyProbe()) {
        stopTopologyProbe();return;
    }
    if(controller_.connecting()) return;
    if(!controller_.confirmed().connected) {stopTopologyProbe();return;}
    if(!topologyReadAt_) topologyReadAt_=now_;
    const auto r=selectedReport();
    const bool current=session_.accepted() && r.twinStatusFresh && now_>=r.twinAtMs && now_-r.twinAtMs<=2000;
    const bool known=current && (r.twinLinkState==0 || (r.twinLinkState==0x12 && !r.twinPeerId.empty()));
    if(known) {
        const bool alone=r.twinLinkState==0;
        topologyProbes_[selected_].confirmedAlone=alone;
        observeTwin(r);
        // One current pair response describes both members, so don't connect
        // to the second member merely to repeat the same topology query.
        if(!alone) markTopologyProbed(r.twinPeerId);
        diagnostic(alone ? "Devices topology confirmed single" : "Devices topology confirmed Twin");
        stopTopologyProbe();return;
    }
    if(now_<*topologyReadAt_ || now_-*topologyReadAt_>=topologyReadDeadlineMs) stopTopologyProbe();
}
bool TrainingApplication::selectionReady() const {
    if(selectionPhase_!=SelectionPhase::Ready || !connected() || !session_.accepted() ||
       session_.readOnlyProbe()!=selectionReadOnlyView_) return false;
    const auto r=report();
    // A previously resolved standalone endpoint retains its navigation and
    // unconditional unload route while ordinary readback is stale. All load/
    // settings operations still enforce their own fresh readiness requirements.
    if(!selectionResolvedTwin_) return twinTransitionPending_ || (!r.twinDetected && !r.twin);
    // Established Primary identity is a route, not current load permission.
    // Its backend-held unload route survives mere polling TTL expiry and is
    // revoked by an actual identity/role/topology contradiction.
    if(!selectionReadOnlyView_) {
        if(!r.unloadRouteKnown || selectionResolvedPeer_.empty() || r.twinPeerId!=selectionResolvedPeer_) return false;
        const bool topologyCurrent=r.twinStatusFresh && now_>=r.twinAtMs && now_-r.twinAtMs<=2000;
        if(topologyCurrent && r.twinLinkState==0) return false;
        const bool roleCurrent=r.twinRoleAtMs!=0 && now_>=r.twinRoleAtMs && now_-r.twinRoleAtMs<=2000;
        return !roleCurrent || r.currentTwinRole==CurrentTwinRole::Master;
    }
    return twinPermissionFresh();
}
bool TrainingApplication::twinPermissionFresh() const {
    if(twinTransitionPending_) return false;
    if(!selectionResolvedTwin_) return true;
    const auto r=report();
    return r.twinDetected && r.twin && r.twinKnown && r.twinStatusFresh && r.twinLinkState==0x12 &&
        !selectionResolvedPeer_.empty() && r.twinPeerId==selectionResolvedPeer_ &&
        r.currentTwinRole==CurrentTwinRole::Master && r.twinRoleAtMs!=0 &&
        now_>=r.twinAtMs && now_-r.twinAtMs<=2000 && now_>=r.twinRoleAtMs && now_-r.twinRoleAtMs<=2000;
}
void TrainingApplication::cancelControlIntents() {
    const bool unloading=session_.unloading();
    session_.cancelTraining();
    if(!unloading) pending_=false;
    requestedLoad_=autoLoadRequested_=false;
    settingsIntent_.reset();weightIntent_.reset();presetIntent_.reset();dropRestore_.reset();dropStart_.reset();
    if(const auto d=dropSettings_.find(selected_);d!=dropSettings_.end()) d->second.enabled=false;
    resetDrops();dropsCancelled_=true;
}
const char* TrainingApplication::connectionEndNotice() const {
    switch(connectionEnd_) {
    case ConnectionEnd::PeerEnded:return "VOLTRA ended the connection.";
    case ConnectionEnd::WorkoutExited:return "Weight Training ended on the VOLTRA.";
    case ConnectionEnd::MountLockReleased:return "VOLTRA reported that the mounting lock was released.";
    case ConnectionEnd::Unknown:return "VOLTRA connection ended.";
    case ConnectionEnd::None:return "";
    }
    return "";
}
void TrainingApplication::refreshConnectedTwinSelection(bool afterProtocolPoll) {
    if(selectionPhase_!=SelectionPhase::Ready || selectionResolvedTwin_ || !connected() || !session_.accepted()) return;
    const auto r=report();
    if(!twinTransitionPending_) {
        if(!r.twinDetected && !r.twin && !session_.checkingTwinAfterWorkoutExit()) return;
        twinTransitionPending_=true;
        cancelControlIntents();
        // Standalone drop/set presentation belongs to the prior scope too.
        dropProgress_={};
        restSince_.reset();restSetRevision_=0;
        confirmedDropWeight_.reset();observedLoaded_=false;
        message_.clear();
        diagnostic("Checking selected connection for Twin transition");
    }
    if(!afterProtocolPoll) return;
    if(!session_.checkingTwinAfterWorkoutExit() && r.twinKnown && !r.twinDetected && !r.twin && fresh()) {
        twinTransitionPending_=false;
        message_.clear();
        return;
    }
    // A pairing indication disables standalone controls immediately. Keep
    // reading the accepted link until its current role and partner are known.
    observeTwin(r);
    refreshTwinDevices();
    const bool paired=r.twin && r.twinKnown && twinFresh() && r.twinLinkState==0x12 &&
        !r.twinPeerId.empty() && !sameDeviceIdentity(r.twinPeerId,selected_) &&
        r.twinRoleAtMs!=0 && now_>=r.twinRoleAtMs && now_-r.twinRoleAtMs<=2000;
    if(!paired) return;
    if(r.currentTwinRole==CurrentTwinRole::Master && r.unloadRouteKnown) {
        // The same authenticated link is now the primary. Keep its explicit
        // unload route while readback catches up to the new combined scope.
        selectionResolvedTwin_=true;
        selectionResolvedPeer_=r.twinPeerId;
        twinTransitionPending_=false;
        diagnostic("Twin transition kept current controller");
        return;
    }
    if(r.currentTwinRole!=CurrentTwinRole::Follower || maintenance_ || session_.busy() || !session_.twinHandoffIdle()) return;
    // The former single endpoint is now a verified idle follower. Reuse
    // bounded read-only recovery and require the primary's reciprocal A7.
    // Navigation evidence never carries a settings or resistance request.
    selectionPhase_=SelectionPhase::Resolving;
    selectionStartedAt_=now_;
    selectionFirstEndpoint_=selected_;
    selectionProbed_={selected_};
    selectionEndpointAccepted_=true;
    selectionTrace_.clear();
    traceSelectionEndpoint();
    recoveryFollower_=selected_;
    retrySelectionCandidate_=r.twinPeerId;
    twinTransitionPending_=false;
    diagnostic("Twin transition connecting to verified controller");
    if(!beginSelectionEndpoint(r.twinPeerId,BleConnectionPolicy::RecoverKnownEndpoint,r.twinPeerName))
        failSelection(message_,transport_.connectionFailure().stage);
}
bool TrainingApplication::retainSelectionControl(bool requireControl) {
    refreshConnectedTwinSelection(false);
    if(resolvingSelection() || selectionFailed()) return false;
    if(selectionReady()) {
        const bool controlsFresh=!selectionReadOnlyView_ && !twinTransitionPending_ && !session_.checkingTwinAfterWorkoutExit() && (!selectionResolvedTwin_ ||
            (twinPermissionFresh() && fresh() && readbackFresh()));
        if(!controlsFresh) {
            cancelControlIntents();
            if(requireControl) {
                waitingForControlFreshness_=!selectionReadOnlyView_;
                message_=selectionReadOnlyView_ ? "Use VOLTRA controls" : "Waiting for VOLTRA";
                recordControlFailure(message_);return false;
            }
        } else if(waitingForControlFreshness_) {
            if(message_=="Waiting for VOLTRA") message_.clear();
            waitingForControlFreshness_=false;
        }
        return true;
    }
    const bool established=selectionPhase_==SelectionPhase::Ready;
    const bool lostLink=established && !connected();
    auto end=established ? ConnectionEnd::Unknown : ConnectionEnd::None;
    auto stage=BleConnectionStage::None;
    std::string reason="VOLTRA connection changed; reconnect";
    if(lostLink) {
        reason="VOLTRA connection lost; reconnect";
        const auto failure=transport_.connectionFailure();
        const auto& protocolReason=session_.status();
        stage=failure.stage;
        // A typed transport event is authoritative. When it has no link code,
        // keep a specific protocol cause such as a failed write or mode exit.
        if(stage!=BleConnectionStage::None) reason=controller_.feedback();
        if((stage==BleConnectionStage::None || (stage==BleConnectionStage::Link && !failure.reason)) &&
            !protocolReason.empty() && protocolReason!="Connection lost") reason=protocolReason;
        if(controller_.endedSessionObservation()==DeviceEndObservation::MountLockReleased) end=ConnectionEnd::MountLockReleased;
        else if(stage==BleConnectionStage::Link && failure.reason==peerTerminatedLinkCode) end=ConnectionEnd::PeerEnded;
        else if(protocolReason=="VOLTRA left Weight Training; reconnect") end=ConnectionEnd::WorkoutExited;
        const auto& snapshot=controller_.endedSessionSnapshot();
        lastClosedSession_=("at="+std::to_string(now_)+" transportStage="+std::to_string(int(stage))+
            " code="+std::to_string(failure.reason)+
            (end==ConnectionEnd::MountLockReleased ? " mountLockRelease=observed " : " ")+snapshot).substr(0,800);
    }
    session_.setReadOnlyProbe(true);
    cancelControlIntents();
    if(established) failSelection(reason,stage,end);
    return false;
}
bool TrainingApplication::canRetrySelection() const {
    if(maintenance_ || resolvingSelection() || connected() || controller_.connecting() || retrySelectionCandidate_.empty()) return false;
    const auto endpoint=reachableSelectionEndpoint(retrySelectionCandidate_);
    return std::any_of(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==endpoint && (d.online || remembered(d.id));});
}
bool TrainingApplication::retrySelection() {
    return canRetrySelection() && select(retrySelectionCandidate_);
}
TrainingReport TrainingApplication::report() const {
    // A topology probe never supplies training, load or set state to the UI.
    // Only the private discovery path may inspect this read-only session.
    return topologyProbeActive_ ? TrainingReport{} : selectedReport();
}
TrainingReport TrainingApplication::selectedReport() const {
    auto r=session_.trainingReport();
    if(!r.twinPeerId.empty()) r.twinPeerId=logicalDeviceId(r.twinPeerId);
    if(!r.twinPeerAddress.empty()) {
        const auto resolve=[&](const DeviceEntry& d) {
            if(sameDeviceIdentity(d.id,selected_) || transport_.deviceAddress(d.id)!=r.twinPeerAddress) return false;
            r.twinPeerId=logicalDeviceId(d.id);r.twinPeerName=d.name;
            if(r.twinPeerName.empty() || r.twinPeerName=="VOLTRA") {
                const auto seen=seenDevices_.find(r.twinPeerId);
                if(seen!=seenDevices_.end() && !seen->second.device.name.empty()) r.twinPeerName=seen->second.device.name;
            }
            return true;
        };
        bool found=false;
        for(const auto& d:recent_) if(resolve(d)) {found=true;break;}
        if(!found) for(const auto& d:knownDevices_) if(resolve(d)) {found=true;break;}
        if(!found) for(const auto& entry:seenDevices_) if(resolve(entry.second.device)) {found=true;break;}
        // A paired peer can stop advertising. Retain its previously resolved
        // display identity only when the current machine address still matches;
        // this name cache does not renew topology or control freshness.
        if(!found) {
            const auto cached=twinLinks_.find(selected_);
            if(cached!=twinLinks_.end() && transport_.deviceAddress(cached->second.peer)==r.twinPeerAddress) {
                r.twinPeerId=cached->second.peer;r.twinPeerName=cached->second.peerName;found=true;
            }
        }
        if(!found) {
            // The adapter can supply a navigation-only identity without
            // inventing a Bluetooth address type for an invisible partner.
            const auto peer=transport_.deviceIdForAddress(r.twinPeerAddress);
            if(!peer.empty() && !sameDeviceIdentity(peer,selected_)) {
                r.twinPeerId=logicalDeviceId(peer);
                if(r.twinPeerName.empty()) r.twinPeerName="VOLTRA";
            }
        }
    }
    return r;
}
bool TrainingApplication::readbackFresh() const {
    const auto r=report();
    return connected() && r.readbackKnown && now_>=r.readbackAtMs && now_-r.readbackAtMs<=2000 &&
        (!r.combinedWeightKnown || (r.twinStatusFresh && now_>=r.twinAtMs && now_-r.twinAtMs<=2000));
}
bool TrainingApplication::twinFresh() const {
    const auto r=report();
    return connected() && r.twinStatusFresh && now_>=r.twinAtMs && now_-r.twinAtMs<=2000;
}
void TrainingApplication::observeTwin(const TrainingReport& r) {
    if(selected_.empty() || !r.twinStatusFresh || now_<r.twinAtMs || now_-r.twinAtMs>2000) return;
    const auto detach=[&](const DeviceId& id) {
        const auto old=twinLinks_.find(id);
        DeviceId peerId=old==twinLinks_.end() ? DeviceId{} : old->second.peer;
        if(peerId.empty()) for(const auto& known:knownDevices_) if(known.id==id) {peerId=known.twinPeerId;break;}
        bool changed=old!=twinLinks_.end();
        if(old!=twinLinks_.end()) {
            const auto peer=twinLinks_.find(old->second.peer);
            if(peer!=twinLinks_.end() && peer->second.peer==id) twinLinks_.erase(peer);
            twinLinks_.erase(old);
        }
        // Fresh machine topology supersedes synthetic/bootstrap navigation too.
        // Otherwise a catalog's old pair would reappear after an Alone reply.
        for(auto& known:knownDevices_) if(known.id==id || (known.id==peerId && known.twinPeerId==id)) {
            changed=changed || !known.twinPeerId.empty() || !known.twinPeer.empty();
            known.twinPeer.clear();known.twinPeerId.clear();known.twinSelectionId.clear();
            known.twinFresh=known.twinPeerAvailable=false;
        }
        return changed;
    };
    if(r.twinLinkState==0) {
        if(detach(selected_)) saveTwinNavigation();
    } else if(r.twinLinkState==0x12 && !r.twinPeerId.empty()) {
        // Transitional identity alone can inform a foreground read-only route,
        // but must not display a newly paired tile or replace confirmed grouping.
        const auto old=twinLinks_.find(selected_),peer=twinLinks_.find(r.twinPeerId);
        // Keep the endpoint that established this relationship. Selecting the
        // other circle must not silently open a second/follower session. This
        // remembered endpoint is navigation only, never proof of host safety.
        DeviceId selection=selected_;
        if(old!=twinLinks_.end() && old->second.peer==r.twinPeerId && !old->second.selection.empty())
            selection=old->second.selection;
        else if(peer!=twinLinks_.end() && peer->second.peer==selected_ && !peer->second.selection.empty())
            selection=peer->second.selection;
        // A fresh backend role can improve the next read-only discovery start
        // even when motor/limit readiness cannot yet promote the session. The
        // saved candidate remains a hint: every new connection rechecks role.
        const bool currentRole=r.twin && r.twinKnown && r.twinLinkState==0x12 && r.twinRoleAtMs!=0 &&
            now_>=r.twinRoleAtMs && now_-r.twinRoleAtMs<=2000;
        if(currentRole && r.currentTwinRole==CurrentTwinRole::Master) selection=selected_;
        else if(currentRole && r.currentTwinRole==CurrentTwinRole::Follower) selection=r.twinPeerId;
        const bool replaceOwn=old!=twinLinks_.end() && old->second.peer!=r.twinPeerId;
        const bool replacePeer=peer!=twinLinks_.end() && peer->second.peer!=selected_;
        const bool metadataChanged=replaceOwn || replacePeer || old==twinLinks_.end() || peer==twinLinks_.end() ||
            old->second.peerName!=r.twinPeerName || peer->second.peerName!=name_ ||
            old->second.selection!=selection || !old->second.navigationKnown;
        if(replaceOwn) detach(selected_);
        if(replacePeer) detach(r.twinPeerId);
        if(!twinLinks_.count(selected_) && twinLinks_.size()+2>maximumRememberedDevices) return;
        twinLinks_[selected_]={r.twinPeerId,r.twinPeerName,r.twinAtMs,true,selection,true};
        twinLinks_[r.twinPeerId]={selected_,name_,r.twinAtMs,true,selection,true};
        if(metadataChanged) saveTwinNavigation();
    }
}
void TrainingApplication::saveTwinNavigation() {
    auto saved=recent_;
    for(const auto& link:twinLinks_) {
        const auto peer=twinLinks_.find(link.second.peer);
        if(!link.second.navigationKnown || peer==twinLinks_.end() || !peer->second.navigationKnown ||
           peer->second.peer!=link.first || peer->second.selection!=link.second.selection) continue;
        const auto present=[&](const DeviceId& id){return std::any_of(saved.begin(),saved.end(),[&](const auto& d){return d.id==id;});};
        const size_t needed=size_t(!present(link.first))+size_t(!present(link.second.peer));
        if(saved.size()+needed>maximumRememberedDevices) {message_="Device memory full";continue;}
        const auto rememberName=[&](const DeviceId& id,const std::string& name){
            const auto entry=std::find_if(saved.begin(),saved.end(),[&](const auto& d){return d.id==id;});
            if(entry==saved.end()) {DeviceEntry d;d.id=id;d.name=name;saved.push_back(std::move(d));}
            else if(!name.empty()) entry->name=name;
        };
        rememberName(link.first,peer->second.peerName);
        rememberName(link.second.peer,link.second.peerName);
    }
    for(auto& d:saved) {
        d.online=d.twinFresh=d.twinPeerAvailable=false;
        d.twinPeer.clear();d.twinPeerId.clear();d.twinSelectionId.clear();
        const auto link=twinLinks_.find(d.id);
        if(link==twinLinks_.end() || !link->second.navigationKnown) continue;
        const auto peer=std::find_if(saved.begin(),saved.end(),[&](const auto& entry){return entry.id==link->second.peer;});
        if(peer==saved.end()) continue;
        d.twinPeerId=peer->id;d.twinPeer=peer->name;d.twinSelectionId=link->second.selection;
    }
    const auto same=[](const DeviceEntry& a,const DeviceEntry& b){
        return a.id==b.id && a.name==b.name && a.twinPeerId==b.twinPeerId &&
            a.twinPeer==b.twinPeer && a.twinSelectionId==b.twinSelectionId;
    };
    if(saved.size()==recent_.size() && std::equal(saved.begin(),saved.end(),recent_.begin(),same)) return;
    if(memory_ && !memory_->save(saved)) {message_="Could not save paired devices";return;}
    recent_=std::move(saved);
}
void TrainingApplication::refreshTwinDevices() {
    const auto r=report();
    const bool currentTwin=connected() && r.twinStatusFresh && now_>=r.twinAtMs && now_-r.twinAtMs<=2000;
    for(auto& d:devices_) {
        const auto it=twinLinks_.find(d.id);
        if(it==twinLinks_.end()) {
            // Synthetic catalog metadata is explicit. Saved physical navigation
            // enters twinLinks_ only after reciprocal identity validation.
            const auto known=std::find_if(knownDevices_.begin(),knownDevices_.end(),[&](const auto& k){return k.id==d.id;});
            if(known==knownDevices_.end() || known->twinPeer.empty()) {
                d.twinPeer.clear();d.twinPeerId.clear();d.twinFresh=d.twinPeerAvailable=false;
                d.twinSelectionId.clear();
            } else {
                d.twinSelectionId=known->twinSelectionId;
                if(d.twinSelectionId.empty()) {
                    // The offline catalog is explicit synthetic metadata. Use
                    // its stable pair order until a session observes the pair.
                    const auto first=std::find_if(knownDevices_.begin(),knownDevices_.end(),[&](const auto& k){
                        return k.id==d.id || (!d.twinPeerId.empty() && k.id==d.twinPeerId);
                    });
                    d.twinSelectionId=first==knownDevices_.end() ? d.id : first->id;
                }
            }
            continue;
        }
        d.twinPeer=it->second.peerName;d.twinPeerId=it->second.peer;
        d.twinSelectionId=it->second.selection;
        const bool currentPeer=(selected_==d.id && r.twinPeerId==it->second.peer) ||
            (selected_==it->second.peer && r.twinPeerId==d.id);
        const bool discoveredFresh=it->second.at && now_>=it->second.at && now_-it->second.at<=topologyPresentationMs;
        d.twinFresh=(currentTwin && currentPeer && now_>=it->second.at && now_-it->second.at<=2000) || discoveredFresh;
        d.twinPeerAvailable=d.twinFresh && it->second.available;
    }
    if(connected() && r.twinDetected && r.twinPeerId.empty()) {
        const auto selected=std::find_if(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==selected_;});
        if(selected!=devices_.end()) {
            selected->twinPeer=r.twinPeerName;
            selected->twinFresh=twinFresh();selected->twinPeerAvailable=selected->twinFresh && r.twinLinkState==0x12;
        }
    }
}
std::optional<int> TrainingApplication::restSeconds() const {
    if(!fresh()) return {};
    if(dropProgress_.active && report().setActive!=false) return {};
    if(!dropProgress_.visible) if(const auto deviceTime=report().restSeconds) return deviceTime;
    if(!restSince_ || now_<*restSince_) return {};
    return int(std::min<uint64_t>((now_-*restSince_)/1000,359999));
}
TrainingApplication::DisplayCounters TrainingApplication::displayCounters() const {
    const auto r=report();
    if(!fresh()) return {};
    if(r.separateTwinDisplayCounters) {
        if(!r.twinDisplayCounters) return {};
        return {r.twinDisplayCounters->set,r.twinDisplayCounters->rep,r.completedSets,r.lastSetReps};
    }
    if(!r.telemetryValid) return {};
    if(!dropProgress_.visible) return {r.setNumber,int(r.rep),r.completedSets,r.lastSetReps};
    if(!dropProgress_.valid) return {};
    const int set=dropProgress_.completedSets+(dropProgress_.active ? 1 : 0);
    const bool resting=dropProgress_.active && r.setActive==false;
    return {set,dropProgress_.reps,
        resting ? std::optional<int>(set) : dropProgress_.completedSets>0 ? std::optional<int>(dropProgress_.completedSets) : std::nullopt,
        resting ? std::optional<int>(dropProgress_.reps) : dropProgress_.completedSets>0 ? std::optional<int>(dropProgress_.lastReps) : std::nullopt};
}
bool TrainingApplication::forget(const DeviceId& requested) {
    const auto id=logicalDeviceId(requested);
    automaticSelection_ = false;
    std::vector<DeviceId> forgotten{id};
    const auto link=twinLinks_.find(id);
    if(link!=twinLinks_.end()) forgotten.push_back(link->second.peer);
    else {
        const auto device=std::find_if(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==id;});
        if(device!=devices_.end() && !device->twinPeerId.empty()) forgotten.push_back(device->twinPeerId);
    }
    const auto includes=[&](const DeviceId& candidate){return std::find(forgotten.begin(),forgotten.end(),candidate)!=forgotten.end();};
    auto remaining = recent_;
    remaining.erase(std::remove_if(remaining.begin(), remaining.end(), [&](const auto& d) { return includes(d.id); }), remaining.end());
    if (memory_ && !memory_->save(remaining)) { message_ = "Could not remove saved device"; return false; }
    recent_ = std::move(remaining);
    // Forget the remote's logical pair record only. No pairing/role command is
    // sent to either VOLTRA.
    for(const auto& member:forgotten) {
        twinLinks_.erase(member);
        dropSettings_.erase(member);
    }
    if (includes(selected_)) leave();
    message_.clear();
    scan();
    return true;
}
bool TrainingApplication::tryAutomaticSelection() {
    if (maintenance_ || !automaticSelection_ || !scanned_ || scanPending_ || connected() || connecting()) return false;
    std::vector<DeviceId> available;
    for(const auto& d:devices_) if(d.online) {
        const auto endpoint=selectionDevice(d.id);
        if(std::find(available.begin(),available.end(),endpoint)==available.end()) available.push_back(endpoint);
    }
    if (available.empty()) return false; // Keep discovering when the remote starts first.
    if (available.size() != 1) {
        automaticSelection_ = false; // Multiple logical choices require a tap.
        return false;
    }
    // Start the read-only check at the currently advertising member. A cached
    // primary identity alone must not impose a failed radio attempt first.
    const auto reachable=reachableSelectionEndpoint(available.front());
    const auto endpoint=std::find_if(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==reachable && d.online;});
    if(endpoint==devices_.end()) return false;
    automaticSelection_ = false; // One startup attempt; never fight manual navigation.
    return startSelection(endpoint->id,false);
}
void TrainingApplication::scan() {
    if (maintenance_ || discoveryPaused_ || radioInUse() || resolvingSelection() || scanPending_) return;
    const auto found=transport_.scan();
    scanPending_=transport_.scanInProgress();
    if (!scanPending_) refreshDiscovery(found);
    scannedAt_ = now_; scanned_ = true;
}
void TrainingApplication::refreshDiscovery(const std::vector<Advertisement>& found) {
    latestAdvertised_.clear();
    latestAdvertisedAt_=now_;
    for(auto it=seenDevices_.begin();it!=seenDevices_.end();) {
        if(now_-it->second.at>=discoveryGraceMs) it=seenDevices_.erase(it);else ++it;
    }
    for (const auto& a : found) {
        if (std::find(a.services.begin(), a.services.end(), VoltraGattProfile::service) == a.services.end()) continue;
        const auto id=logicalDeviceId(a.id);
        if(seenDevices_.size()>=maximumDiscoveredDevices && !seenDevices_.count(id)) continue;
        auto& seen=seenDevices_[id];seen.device.id=id;seen.device.online=true;seen.at=now_;
        if(std::find(latestAdvertised_.begin(),latestAdvertised_.end(),id)==latestAdvertised_.end()) latestAdvertised_.push_back(id);
        if(seen.device.name.empty() || seen.device.name=="VOLTRA") {
            const auto saved=std::find_if(recent_.begin(),recent_.end(),[&](const auto& d){return d.id==id;});
            const auto known=std::find_if(knownDevices_.begin(),knownDevices_.end(),[&](const auto& d){return d.id==id;});
            if(saved!=recent_.end() && !saved->name.empty() && saved->name!="VOLTRA") seen.device.name=saved->name;
            else if(known!=knownDevices_.end() && !known->name.empty() && known->name!="VOLTRA") seen.device.name=known->name;
            else if(!a.name.empty()) seen.device.name=a.name;
            else if(saved!=recent_.end()) seen.device.name=saved->name;
            else if(known!=knownDevices_.end()) seen.device.name=known->name;
        }
        if(seen.device.name.empty()) seen.device.name="VOLTRA";
    }
    devices_ = knownDevices_;
    for(const auto& d:recent_) {
        const auto known=std::find_if(devices_.begin(),devices_.end(),[&](const auto& entry){return entry.id==d.id;});
        if(known==devices_.end()) devices_.push_back(d);
        else {
            if(!d.name.empty() && (d.name!="VOLTRA" || known->name.empty())) known->name=d.name;
            if(!d.twinPeerId.empty()) {known->twinPeer=d.twinPeer;known->twinPeerId=d.twinPeerId;known->twinSelectionId=d.twinSelectionId;}
        }
    }
    for (auto& d : devices_) d.online = false;
    for (const auto& pair : seenDevices_) {
        const auto& a=pair.second.device;
        auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == a.id; });
        if (it == devices_.end()) devices_.push_back({a.id, a.name.empty() ? "VOLTRA" : a.name, true, {}});
        else { if(it->name.empty() || it->name=="VOLTRA") it->name = a.name; it->online = true; }
    }
    refreshTwinDevices();
    topologyProbeDue_=true;
}
bool TrainingApplication::select(const DeviceId& id) {
    return startSelection(id,true);
}
bool TrainingApplication::startSelection(const DeviceId& id, bool userSelected) {
    if(maintenance_) return false;
    const auto endpoint=reachableSelectionEndpoint(id);
    const auto it = std::find_if(devices_.begin(), devices_.end(), [&](const auto& d) { return d.id == endpoint && (d.online || remembered(d.id)); });
    if (it == devices_.end()) return false; // An inactive tap has no side effects.
    leave();
    userSelected_=userSelected;
    selectionPhase_=SelectionPhase::Resolving;selectionStartedAt_=now_;
    selectionFirstEndpoint_=endpoint;
    retrySelectionCandidate_=logicalDeviceId(id);
    selectionTrace_.clear();selectionFirstFailure_.clear();
    const auto link=twinLinks_.find(endpoint);
    if(link!=twinLinks_.end()) selectionPeerCandidate_=link->second.peer;
    if(!beginSelectionEndpoint(endpoint)) {failSelection(message_,transport_.connectionFailure().stage);return false;}
    return true;
}
bool TrainingApplication::beginSelectionEndpoint(const DeviceId& id, BleConnectionPolicy policy,
                                                  const std::string& peerName) {
    const bool recovering=policy==BleConnectionPolicy::RecoverKnownEndpoint;
    if(!recovering && (selectionProbed_.size()>=2 ||
       std::find(selectionProbed_.begin(),selectionProbed_.end(),id)!=selectionProbed_.end())) return false;
    const auto it=std::find_if(devices_.begin(),devices_.end(),[&](const auto& d){return d.id==id && (d.online || remembered(id));});
    const auto saved=std::find_if(recent_.begin(),recent_.end(),[&](const auto& d){return d.id==id;});
    DeviceEntry chosen;
    if(it!=devices_.end()) chosen=*it;
    else if(saved!=recent_.end()) chosen=*saved;
    else if(recovering) {
        // Fresh follower status supplies identity. The transport still
        // requires a known radio address type before attempting a connection.
        chosen.id=id;
        chosen.name=peerName.empty() ? "VOLTRA" : peerName;
    } else {message_="Paired VOLTRA is unavailable";return false;}
    if(!session_.setReadOnlyProbe(true)) {message_="Connection checking is unavailable";return false;}
    selectionProbed_.push_back(id);selectionEndpointStartedAt_=now_;selectionReading_=false;selectionReadAt_=0;
    selectionEndpointAccepted_=false;
    connectingDevice_.reset();
    // Keep both known names on screen during handoff. The display names
    // are independent of which physical endpoint is being attempted.
    const bool knownHandoff=recovering || (selectionFirstPeerObserved_ &&
        sameDeviceIdentity(selected_,selectionFirstEndpoint_) && sameDeviceIdentity(id,selectionExpectedPeer_));
    const auto& displayName=recovering && (chosen.name.empty() || chosen.name=="VOLTRA") && !peerName.empty()
        ? peerName : chosen.name;
    selectionStatus_=knownHandoff && sameDeviceIdentity(selected_,selectionFirstEndpoint_)
        ? connectingStatus(name_,displayName)
        : connectingStatus(displayName,knownHandoff ? name_ : chosen.twinPeer);
    selected_=chosen.id;name_=chosen.name;
    if(!controller_.connect(chosen.id,policy)) {message_=controller_.feedback();return false;}
    session_.setSelectedAddress(transport_.deviceAddress(chosen.id));
    connectingDevice_=chosen;
    message_="Connecting...";
    return true;
}
void TrainingApplication::traceSelectionEndpoint() {
    if(selected_.empty()) return;
    const auto elapsed=now_>=selectionEndpointStartedAt_ ? now_-selectionEndpointStartedAt_ : 0;
    std::string detail;
    if(!selectionEndpointAccepted_ && !session_.accepted()) {
        const auto failure=transport_.connectionFailure();
        detail="accepted=0 readOnlyProbe="+std::to_string(session_.readOnlyProbe())+
            " transportStage="+std::to_string(static_cast<int>(failure.stage))+" code="+std::to_string(failure.reason)+
            " timedOut="+std::to_string(failure.timedOut)+" attempts="+std::to_string(failure.attempts)+
            " radioElapsedMs="+std::to_string(failure.elapsedMs)+" logicalType="+std::to_string(failure.logicalAddressType)+
            " radioType="+std::to_string(failure.radioAddressType)+" typeObserved="+std::to_string(failure.observedRadioType);
    } else detail=session_.diagnosticState().substr(0,700);
    const auto snapshot="candidate"+std::to_string(selectionProbed_.size())+" elapsedMs="+std::to_string(elapsed)+
        "={"+detail+"}";
    if(!selectionTrace_.empty()) selectionTrace_+=' ';
    selectionTrace_+=snapshot;
    if(selectionTrace_.size()>1450) selectionTrace_.resize(1450);
}
void TrainingApplication::failSelection(const std::string& reason,BleConnectionStage stage,ConnectionEnd end) {
    const auto failure=reason.empty() ? std::string("Could not identify the paired controller") : reason;
    traceSelectionEndpoint();
    const auto elapsed=now_>=selectionStartedAt_ ? now_-selectionStartedAt_ : 0;
    lastConnectionFailure_=("at="+std::to_string(now_)+" elapsedMs="+std::to_string(elapsed)+" reason="+failure).substr(0,180);
    diagnostic("Connection selection failed: "+failure);
    leave();message_=failure;selectionPhase_=SelectionPhase::Failed;
    connectionEnd_=end;
    // A failed GAP connection can have several causes. Give troubleshooting
    // advice separately from the actual stage and code recorded above.
    suggestDisconnectOtherBluetooth_=end==ConnectionEnd::None && stage==BleConnectionStage::Radio;
}
bool TrainingApplication::runSelection() {
    if(selectionPhase_==SelectionPhase::Ready) {
        // The protocol has just polled on this tick's clock. Only this path
        // can use a fresh local-idle proof to move the Bluetooth link.
        refreshConnectedTwinSelection(true);
        return !retainSelectionControl(false) || twinTransitionPending_ || session_.checkingTwinAfterWorkoutExit();
    }
    if(selectionPhase_!=SelectionPhase::Resolving) return false;
    const bool recovering=!recoveryFollower_.empty();
    const auto deadline=recovering ? twinRecoveryDeadlineMs : selectionDeadlineMs;
    if(now_<selectionStartedAt_ || now_-selectionStartedAt_>=deadline) {
        if(recovering) {
            if(controller_.connecting()) failSelection("Bluetooth radio connection timed out",BleConnectionStage::Radio);
            else failSelection("Could not verify the paired controller");
            return true;
        }
        // An observed topology contradiction remains more specific than a
        // timer or an earlier radio failure, including on the deadline tick.
        const auto r=report();
        const bool identityFresh=connected() && session_.accepted() && r.twinStatusFresh &&
            now_>=r.twinAtMs && now_-r.twinAtMs<=2000 && !r.twinPeerId.empty();
        if(selectionProbed_.size()>1 && identityFresh && (r.twinPeerId!=selectionFirstEndpoint_ ||
           (selectionFirstPeerObserved_ && selected_!=selectionExpectedPeer_))) {
            failSelection("VOLTRA pair changed while connecting");return true;
        }
        const auto stage=selectionFirstFailureStage_;
        failSelection(stage!=BleConnectionStage::None ? selectionFirstFailure_ : "Could not identify the paired controller",stage);
        return true;
    }
    if(controller_.connecting()) return true;
    if(!connected()) {
        const auto failure=transport_.connectionFailure();
        if(recovering) {failSelection(controller_.feedback(),failure.stage);return true;}
        if(selectionFirstFailure_.empty()) {
            selectionFirstFailure_=std::string(controller_.feedback()).substr(0,160);
            selectionFirstFailureStage_=failure.stage;
        }
        if(selectionProbed_.size()==1 && !selectionFirstAccepted_ && failure.stage!=BleConnectionStage::None)
            selectionFirstFailedBeforeAcceptance_=true;
        if(!selectionPeerCandidate_.empty() && std::find(selectionProbed_.begin(),selectionProbed_.end(),selectionPeerCandidate_)==selectionProbed_.end()) {
            traceSelectionEndpoint();
            if(beginSelectionEndpoint(selectionPeerCandidate_)) return true;
            failSelection(message_,transport_.connectionFailure().stage);return true;
        }
        failSelection(controller_.feedback(),failure.stage);return true;
    }
    if(!selectionReading_) {selectionReadAt_=now_;selectionReading_=true;}
    if(session_.accepted()) {
        selectionEndpointAccepted_=true;
        if(selectionProbed_.size()==1) selectionFirstAccepted_=true;
    }
    const auto r=report();
    // Full fresh identity can nominate a read-only candidate while link-state
    // semantics remain unknown. It cannot promote that candidate to controls.
    const bool topologyFresh=session_.accepted() && r.twinStatusFresh && now_>=r.twinAtMs && now_-r.twinAtMs<=2000;
    const bool identityFresh=topologyFresh && !r.twinPeerId.empty();
    const bool paired=identityFresh && r.twinDetected && r.twin && r.twinKnown && r.twinLinkState==0x12;
    if(identityFresh) {
        if(recovering) {
            if(!sameDeviceIdentity(r.twinPeerId,recoveryFollower_)) {
                failSelection("VOLTRA pair changed while connecting");return true;
            }
        } else if(selectionProbed_.size()==1) {
            selectionExpectedPeer_=r.twinPeerId;selectionFirstPeerObserved_=true;
            selectionPeerCandidate_=r.twinPeerId;
        } else if(r.twinPeerId!=selectionFirstEndpoint_ ||
                  (selectionFirstPeerObserved_ && selected_!=selectionExpectedPeer_)) {
            failSelection("VOLTRA pair changed while connecting");return true;
        }
    }
    if(paired) {
        const auto peerName=r.twinPeerName.empty() ? "VOLTRA" : r.twinPeerName;
        selectionStatus_=sameDeviceIdentity(r.twinPeerId,selectionFirstEndpoint_)
            ? connectingStatus(peerName,name_) : connectingStatus(name_,peerName);
    }
    else if(topologyFresh && r.twinLinkState==0) selectionStatus_=connectingStatus(name_);
    if(session_.accepted()) {observeTwin(r);refreshTwinDevices();}
    const bool roleFresh=paired && r.currentTwinRole==CurrentTwinRole::Master && r.twinRoleAtMs!=0 &&
        now_>=r.twinRoleAtMs && now_-r.twinRoleAtMs<=2000;
    const bool single=session_.accepted() && selectionProbed_.size()==1 && readbackFresh() &&
        r.twinKnown && !r.twin && !r.twinDetected;
    bool promoted=false,readOnlyMaster=false;
    if(roleFresh || single) {
        promoted=session_.setReadOnlyProbe(false) && !session_.readOnlyProbe();
        // A verified read-only route can show the pair while motor/limit
        // authorization remains unavailable. Retain the backend probe gate.
        readOnlyMaster=roleFresh && !promoted && session_.readOnlyProbe();
    }
    if(promoted || readOnlyMaster) {
        if(selectionProbed_.size()>1) traceSelectionEndpoint();
        selectionReadOnlyView_=readOnlyMaster;
        selectionResolvedTwin_=roleFresh;selectionResolvedPeer_=roleFresh ? r.twinPeerId : DeviceId{};
        if(roleFresh) {
            const auto own=twinLinks_.find(selected_);
            if(own!=twinLinks_.end()) {
                const auto peer=twinLinks_.find(own->second.peer);
                own->second.selection=selected_;
                if(peer!=twinLinks_.end()) peer->second.selection=selected_;
                saveTwinNavigation();refreshTwinDevices();
            }
        }
        selectionPhase_=SelectionPhase::Ready;message_.clear();selectionStatus_.clear();return false;
    }
    const bool follower=paired && r.currentTwinRole==CurrentTwinRole::Follower && r.twinRoleAtMs!=0 &&
        now_>=r.twinRoleAtMs && now_-r.twinRoleAtMs<=2000;
    if(recovering) {
        if(follower || now_-selectionReadAt_>=selectionReadDeadlineMs)
            failSelection("Could not verify the paired controller");
        return true;
    }
    const bool verifiedPeerPreviouslyFailed=selectionProbed_.size()==2 && selectionFirstFailedBeforeAcceptance_ &&
        selectionFirstFailureStage_==BleConnectionStage::Radio && r.twinPeerId==selectionFirstEndpoint_;
    if(userSelected_ && follower && (selectionProbed_.size()==1 || verifiedPeerPreviouslyFailed)) {
        // Steven's known-address recovery targets the controller only. This
        // follows current own-matched follower evidence and permits no writes
        // until the controller reports the reciprocal, freshly verified pair.
        traceSelectionEndpoint();
        recoveryFollower_=selected_;
        selectionStartedAt_=now_;
        diagnostic("Twin controller recovery started");
        if(!beginSelectionEndpoint(r.twinPeerId,BleConnectionPolicy::RecoverKnownEndpoint,r.twinPeerName))
            failSelection(message_,transport_.connectionFailure().stage);
        return true;
    }
    if(follower && selectionProbed_.size()==2 && selectionFirstFailedBeforeAcceptance_ && selectionFirstFailureStage_!=BleConnectionStage::None &&
       r.twinPeerId==selectionFirstEndpoint_) {
        // The peer is identified, but its earlier connection failed. Preserve
        // that real failure; revisiting it here would stack another retry loop.
        failSelection(selectionFirstFailure_,selectionFirstFailureStage_);return true;
    }
    if(follower || now_-selectionReadAt_>=selectionReadDeadlineMs) {
        if(!selectionPeerCandidate_.empty() && std::find(selectionProbed_.begin(),selectionProbed_.end(),selectionPeerCandidate_)==selectionProbed_.end()) {
            traceSelectionEndpoint();
            if(beginSelectionEndpoint(selectionPeerCandidate_)) return true;
            failSelection(message_,transport_.connectionFailure().stage);return true;
        }
        failSelection(paired ? "Could not identify the paired controller" : "Could not read VOLTRA state");return true;
    }
    return true;
}
void TrainingApplication::rememberConnection() {
    if (!connectingDevice_) return;
    const auto chosen = *connectingDevice_;
    connectingDevice_.reset();
    if (remembered(selected_) || recent_.size() < maximumRememberedDevices) {
        auto saved = recent_;
        saved.erase(std::remove_if(saved.begin(), saved.end(), [&](const auto& d) { return d.id == selected_; }), saved.end());
        saved.insert(saved.begin(), chosen);
        if (memory_ && !memory_->save(saved)) message_ = "Could not save device";
        else recent_ = std::move(saved);
    } else message_ = "Device memory full";
    scan();
}
void TrainingApplication::leave(bool preserveFailure) {
    if(topologyProbeActive_) stopTopologyProbe();
    if(connected()) lastClosedSession_=("at="+std::to_string(now_)+" "+session_.diagnosticState()).substr(0,800);
    const std::string failure = preserveFailure ? (!message_.empty() ? message_ : session_.status().empty() ? "Connection lost" : session_.status()) : "";
    if(!preserveFailure) {suggestDisconnectOtherBluetooth_=false;connectionEnd_=ConnectionEnd::None;}
    automaticSelection_ = false;
    selectionPhase_=SelectionPhase::Idle;selectionStartedAt_=selectionEndpointStartedAt_=selectionReadAt_=0;selectionReading_=false;
    selectionStatus_.clear();
    userSelected_=false;recoveryFollower_.clear();
    selectionEndpointAccepted_=selectionFirstAccepted_=selectionFirstFailedBeforeAcceptance_=false;
    selectionFirstFailureStage_=BleConnectionStage::None;
    selectionProbed_.clear();selectionPeerCandidate_.clear();
    selectionFirstEndpoint_.clear();selectionExpectedPeer_.clear();selectionResolvedPeer_.clear();
    selectionFirstPeerObserved_=selectionResolvedTwin_=false;
    selectionReadOnlyView_=false;
    twinTransitionPending_=false;
    waitingForControlFreshness_=false;
    scanned_ = scanPending_ = false;
    topologyProbeDue_=false;
    connectingDevice_.reset();
    presetIntent_.reset(); observedLoaded_ = false;
    restSince_.reset();restSetRevision_=0;
    dropProgress_={};
    confirmedDropWeight_.reset();
    dropStart_.reset();dropRestore_.reset();
    if (!selected_.empty()) dropSettings_[selected_].enabled = false;
    controller_.disconnect(); selected_.clear(); name_.clear();
    pending_ = requestedLoad_ = autoLoadRequested_ = false; weightIntent_.reset(); settingsIntent_.reset(); message_ = failure; resetDrops();
}
bool TrainingApplication::fresh() const {
    const auto r = report();
    return connected() && r.revision != 0 && now_ >= r.receivedAtMs && now_ - r.receivedAtMs <= 2000;
}
bool TrainingApplication::settingsAllowed() const {
    const auto r = report();
    return selectionReady() && !selectionReadOnlyView_ && twinPermissionFresh() && fresh() && readbackFresh() &&
        r.twinKnown && session_.readiness().validated &&
        r.minWeight > 0 && weightTicksDown(r,r.maxWeight) >= weightTicksUp(r,r.minWeight) && !r.autoLoadArmed && !requestedLoad_;
}
void TrainingApplication::sent(TransportResult result) {
    waitingForControlFreshness_=false; // New request feedback owns the message now.
    if (result != TransportResult::Accepted) { message_ = session_.status().empty() ? "Request not accepted" : session_.status(); return; }
    pending_ = true; sentAt_ = now_; sentRevision_ = report().revision;
    message_.clear(); // Brief requests do not flash a warning on every input.
}
bool TrainingApplication::sendSettings(TrainingSettings s,bool explicitBaseEdit) {
    if(!retainSelectionControl()) return false;
    const auto r = report();
    if(r.loaded && !r.liveWeightSupported && (!explicitBaseEdit || !r.manualLiveWeightSupported ||
       s.magicMode!=r.settings.magicMode || !sameModifier(s.magic,r.settings.magic) || !sameModifier(s.eccentric,r.settings.eccentric))) {
        message_="Unload to adjust";return false;
    }
    if (!settingsAllowed() || !modifierUnitsAllowed(r,s) || s.weight < r.minWeight || !weightOnIncrement(r,s.weight) || s.magic.value < 0 || s.eccentric.value < 0 ||
        (s.magic.enabled && s.magic.value == 0) || (s.eccentric.enabled && s.eccentric.value == 0) ||
        (s.magic.enabled && !s.magic.percent && !weightOnIncrement(r,s.magic.value)) ||
        (s.eccentric.enabled && !s.eccentric.percent && !weightOnIncrement(r,s.eccentric.value)) ||
        (s.magic.enabled && s.magic.value > modifierMaximum(r, s, true)) ||
        (s.eccentric.enabled && s.eccentric.value > modifierMaximum(r, s, false)) ||
        envelopeHundredths(s) > r.maxWeight * 100) { message_ = "Outside device limits"; return false; }
    const auto result = session_.setTraining(s); sent(result);
    if (result == TransportResult::Accepted) settingsIntent_ = s;
    return result == TransportResult::Accepted;
}
void TrainingApplication::adjustWeight(int delta) {
    if(!retainSelectionControl()) return;
    if(dropRestore_) {message_="Restoring starting weight; please wait";return;}
    if (presetIntent_) { message_ = "Awaiting preset confirmation"; return; }
    if (!settingsAllowed()) {
        const auto reason=session_.readiness().reason;
        message_ = reason.empty() ? "Device state unavailable" : reason;
        recordControlFailure(message_);return;
    }
    auto r = report();
    if (r.loaded && !r.liveWeightSupported && !r.manualLiveWeightSupported) { message_ = "Unload to adjust"; return; }
    dropStart_.reset(); // A subsequent manual base choice supersedes restoration.
    if(running_ || dropTarget_ || dropsExhausted_) stopDrops("Drops stopped for manual adjustment");
    const bool loadedBaseOnly=r.loaded && !r.liveWeightSupported && r.manualLiveWeightSupported;
    auto s = loadedBaseOnly ? r.settings : settingsIntent_.value_or(r.settings);
    const int minimum=weightTicksUp(r,r.minWeight),ceiling=weightTicksDown(r,r.loaded ? baseMaximum(r) : r.maxWeight);
    if(ceiling<minimum) {message_="Outside device limits";return;}
    const int requested=(weightIntent_ && weightIntentUnit_==r.unit ? *weightIntent_ : s.weight)+delta;
    const int aligned=delta>0 ? weightTicksUp(r,requested) : delta<0 ? weightTicksDown(r,requested) : weightTicksNearest(r,requested);
    s.weight = std::clamp(aligned, minimum, ceiling);
    // The original brief specifies reducing modifiers before capping the base.
    // Trim EO first, then magic; this is part of this explicit knob request.
    for (auto* m : {&s.eccentric, &s.magic}) {
        const int divisor = m->percent ? s.weight : 100;
        const int excess = envelopeHundredths(s) - r.maxWeight*100;
        if (excess > 0 && m->enabled && divisor > 0)
            m->value = std::max(0, m->value - (excess+divisor-1)/divisor);
        if(!m->percent) m->value=weightTicksDown(r,m->value);
        if(m->enabled && m->value == 0) m->enabled=false;
    }
    if (!pending_ && s.weight == r.settings.weight && sameModifier(s.magic,r.settings.magic) &&
        sameModifier(s.eccentric,r.settings.eccentric)) return;
    if (sendSettings(s,true)) {weightIntent_ = s.weight;weightIntentUnit_=r.unit;}
}
void TrainingApplication::recordControlFailure(const std::string& reason) {
    // RAM-only boot-local snapshot survives leaving Devices for Settings. The
    // session itself clears on disconnect, which used to lose the useful values.
    lastControlFailure_=("at="+std::to_string(now_)+" reason="+reason+" "+session_.diagnosticState()).substr(0,900);
    diagnostic("Control failure: "+reason);
}
void TrainingApplication::recordLoadRefusal() {
    loadRefusalMessage_=message();++loadRefusalRevision_;
    recordControlFailure(loadRefusalMessage_);
}
void TrainingApplication::toggleWeight() {
    if(!retainSelectionControl(false)) return;
    diagnostic("Explicit weight action");
    auto r = report();
    if (!fresh() || !twinPermissionFresh() || r.loaded || r.autoLoadArmed || requestedLoad_) {
        unload(); return;
    }
    if(!retainSelectionControl()) return;
    if(session_.unloading()) {
        // A second tap during the device's reset is not a new load intent.
        // Keep the pending feedback, without opening a failure dialog or replay.
        message_.clear();diagnostic("Weight action ignored: unload pending");return;
    }
    const auto e = session_.readiness();
    if (pending() || !settingsAllowed() || !e.ready || !e.cableZero || !e.twinSafe || r.settings.weight < r.minWeight) {
        message_ = r.minWeight>0 && r.settings.weight < r.minWeight ? "Set at least "+weightText(r.minWeight,r.unit)+" "+unitName(r.unit) :
            !e.reason.empty() ? e.reason : "Device not ready"; recordLoadRefusal();return;
    }
    const auto result = session_.setLoaded(true);
    autoLoadRequested_=false;
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
    diagnostic("Load request result="+std::to_string(int(result)));
    if(result!=TransportResult::Accepted) recordLoadRefusal();
}
void TrainingApplication::unload() {
    if(!retainSelectionControl(false)) return;
    const auto r=report();
    if(selectionReadOnlyView_ || ((selectionResolvedTwin_ || r.twinDetected || r.twin) && !r.unloadRouteKnown)) {
        message_="Use VOLTRA to unload";recordControlFailure(message_);return;
    }
    diagnostic("Unload requested");
    // Always a cancel/unload, even before a pending load is reported. Never
    // route an editor's unload target through a toggle that could load instead.
    presetIntent_.reset(); settingsIntent_.reset(); resetDrops(); dropsCancelled_ = true;
    dropRestore_.reset(); // Explicit unload cancels any already queued restore.
    requestedLoad_ = autoLoadRequested_ = false; weightIntent_.reset();
    const auto result=session_.setLoaded(false);
    sent(result);
    diagnostic("Unload request result="+std::to_string(int(result)));
    if(result!=TransportResult::Accepted) recordControlFailure(message_.empty() ? "Unload request not accepted" : message_);
}
const char* TrainingApplication::autoLoadUnavailableText(const TrainingReport& r) {
    if(r.autoLoadEnabled==false) return "Auto Load off on VOLTRA";
    if(r.autoLoadSafetyEnabled==false) return "Safety Check off on VOLTRA";
    return "Auto Load unavailable";
}
void TrainingApplication::autoLoad() {
    if(!retainSelectionControl()) return;
    diagnostic("Explicit Auto Load action");
    if(session_.unloading()) {
        message_.clear();diagnostic("Auto Load action ignored: unload pending");return;
    }
    const auto r = report(); const auto e = session_.readiness();
    if (!settingsAllowed() || pending_ || r.loaded || !r.autoLoadSupported || !e.ready || !e.twinSafe || r.settings.weight < r.minWeight) {
        message_ = e.reason.empty() ? autoLoadUnavailableText(r) : e.reason; recordLoadRefusal();return;
    }
    const auto result = session_.triggerAutoLoad();
    // A new explicit Auto Load starts a new execution sequence, preserving the
    // selected local recipe. Countdown reports cannot inherit an old set/hold.
    if(result == TransportResult::Accepted) resetDrops();
    autoLoadRequested_ = result == TransportResult::Accepted;
    requestedLoad_ = result == TransportResult::Accepted; sent(result);
    diagnostic("Auto Load request result="+std::to_string(int(result)));
    if(result!=TransportResult::Accepted) recordLoadRefusal();
}
void TrainingApplication::toggleModifier(bool magic) {
    if(!retainSelectionControl()) return;
    const auto r = report();
    if (dropRestore_ || presetIntent_ || (pending_ && (!settingsIntent_ || r.loaded)) || (r.loaded && !r.liveModifierAmountsSupported) ||
        !(magic ? r.magicSupported[int(r.settings.magicMode)] : r.eccentricSupported)) {
        message_ = r.loaded ? "Wait for modifier controls" : "Awaiting device"; return;
    }
    auto s = settingsIntent_.value_or(r.settings); auto& m = magic ? s.magic : s.eccentric;
    m.enabled = !m.enabled;
    sendSettings(s);
}
Preset TrainingApplication::currentPreset() const {
    const auto r = report();
    return {r.settings.magicMode, r.settings.magic, r.settings.eccentric, r.unit, drops()};
}
Preset TrainingApplication::presetForCurrentDevice(Preset p) const {
    const auto r=report();
    const auto target = r.unit;
    if(!p.magic.percent) p.magic.value = convertWeightUnits(p.magic.value,p.unit,target);
    if(!p.eccentric.percent) p.eccentric.value = convertWeightUnits(p.eccentric.value,p.unit,target);
    p.drops.step = convertWeightUnits(p.drops.step,p.unit,target);
    for(auto* modifier:{&p.magic,&p.eccentric}) if(!modifier->percent) {
        modifier->value=weightTicksNearest(r,modifier->value);
        if(modifier->enabled) modifier->value=std::max(weightIncrementTicks(r),modifier->value);
    }
    const int direction=p.drops.step<0 ? -1 : 1;
    p.drops.step=direction*weightTicksNearest(r,std::abs(p.drops.step));
    p.unit = target;
    return p;
}
bool TrainingApplication::applyPreset(const Preset& saved) {
    if(!retainSelectionControl()) return false;
    const auto r = report(); presetError_.clear();
    if(!fresh()) { message_ = "Device state unavailable"; return false; }
    auto p = presetForCurrentDevice(saved);
    if(r.sharedModifierUnits) {
        if(p.magic.enabled && p.eccentric.enabled && p.magic.percent!=p.eccentric.percent) {
            presetError_="This VOLTRA shares the Magic / EO unit";message_=presetError_;return false;
        }
        const bool percent=p.magic.enabled ? p.magic.percent : p.eccentric.enabled ? p.eccentric.percent : r.settings.magic.percent;
        if(!p.magic.enabled) p.magic.percent=percent;
        if(!p.eccentric.enabled) p.eccentric.percent=percent;
    }
    if ((p.magic.enabled && !r.magicSupported[int(p.mode)]) ||
        (((p.magic.enabled && p.magic.percent) || (p.eccentric.enabled && p.eccentric.percent)) && !r.percentageSupported) ||
        (p.eccentric.enabled && !r.eccentricSupported) || (p.drops.enabled && !r.liveWeightSupported)) {
        presetError_ = "Preset values not available on Voltra firmware version "+
            (r.firmwareVersion.empty() ? "unknown" : r.firmwareVersion);
        message_ = presetError_; return false;
    }
    if (pending() || r.loaded || std::abs(p.drops.step) > r.maxWeight) {
        message_ = "Unload / wait before applying preset"; return false;
    }
    if(p.drops.enabled && p.drops.mode==DropMode::RestPause &&
       (restPauseHoldMaximum(r)<1 || p.drops.holdSeconds>restPauseHoldMaximum(r))) {
        presetError_=restPauseLimitError;message_=presetError_;return false;
    }
    if(r.twin && !r.liveWeightSupported && !p.drops.enabled) {
        // An OFF recipe carries no supported Twin drop configuration. Keep the
        // local OFF recipe rather than staging an unsupported dormant edit.
        p.drops=drops();p.drops.enabled=false;
    }
    // A preset replaces all modifiers as one request, using the same per-device
    // unit/increment conversion shown in its preview. The stored recipe stays
    // unchanged; normal limits still apply. Lower only base for headroom.
    auto s = r.settings; s.magicMode = p.mode; s.magic = p.magic; s.eccentric = p.eccentric;
    auto limits = r; limits.settings = s;
    s.weight = std::min(s.weight,baseMaximum(limits));
    if (!sendSettings(s)) return false;
    presetIntent_ = p; return true;
}
bool TrainingApplication::applyModifier(bool magic, MagicMode mode, int value, bool percent) {
    if(!retainSelectionControl()) return false;
    const auto r = report();
    if (dropRestore_ || presetIntent_ || (pending_ && (!settingsIntent_ || r.loaded)) || (r.loaded && !r.liveModifierAmountsSupported) || !(magic ? r.magicSupported[int(mode)] : r.eccentricSupported)) {
        message_ = r.loaded ? "Wait for modifier controls" : "Awaiting device"; return false;
    }
    if(r.loaded && ((magic && mode!=r.settings.magicMode) || percent!=(magic ? r.settings.magic.percent : r.settings.eccentric.percent))) {
        message_="Unload to switch curve or units";return false;
    }
    if (value < 1) { message_ = "No headroom for modifier"; return false; }
    auto s = settingsIntent_.value_or(r.settings); auto& m = magic ? s.magic : s.eccentric;
    if (magic) s.magicMode = mode;
    m.enabled = true; m.value = value; m.percent = percent;
    if(r.sharedModifierUnits) {
        auto& other=magic ? s.eccentric : s.magic;
        convertModifierUnit(other,percent,s.weight);
        if(!percent) {
            other.value=weightTicksNearest(r,other.value);
            if(other.enabled) other.value=std::max(weightIncrementTicks(r),other.value);
        }
    }
    return sendSettings(s);
}
DropSettings TrainingApplication::drops() const {
    const auto it = dropSettings_.find(selected_);
    if (it != dropSettings_.end()) return it->second;
    auto d = DropSettings{}; d.step *= ticksPerUnit(report().unit); return d;
}
std::optional<int> TrainingApplication::restPauseRemainingPermille() const {
    const auto r=report(); const auto d=drops();
    if(!fresh() || !r.loaded || !r.telemetryValid || r.autoLoadArmed || !r.held || r.moving ||
       !d.enabled || d.mode!=DropMode::RestPause || d.percent<=0 || d.drops<=dropsCompleted_ ||
       d.holdSeconds<1 || d.holdSeconds>restPauseHoldMaximum(r) || !running_ || !movementSeen_ ||
       holdConsumed_ || dropsCancelled_ || dropsExhausted_ || waitingForDropSet_ || pending() || dropTarget_ ||
       !holdSince_ || now_<holdSince_ || !r.motionRevision || now_<r.motionAtMs || now_-r.motionAtMs>motionFreshMs)
        return {};
    const auto elapsed=now_-holdSince_;
    if(elapsed<restPauseBarDelayMs) return {};
    const uint64_t duration=uint64_t(d.holdSeconds)*1000;
    return int((duration-std::min(elapsed,duration))*1000/duration);
}
bool TrainingApplication::applyDrops(DropSettings d) {
    if(!retainSelectionControl()) return false;
    if (!fresh() || selected_.empty()) { message_ = "Device state unavailable"; return false; }
    const auto current=report();
    if(current.twin && !current.liveWeightSupported) {
        const auto previous=drops();
        if(d.enabled || d.mode!=previous.mode || d.step!=previous.step || d.drops!=previous.drops ||
           d.percent!=previous.percent || d.holdSeconds!=previous.holdSeconds) {
            message_="Drops unavailable for this VOLTRA";return false;
        }
    }
    if(d.enabled && (!report().liveWeightSupported || report().minWeight<1 || report().autoLoadArmed || pending())) {
        message_="Wait for weight controls before arming drops";return false;
    }
    if(d.enabled && d.mode==DropMode::RestPause &&
       (restPauseHoldMaximum(report())<1 || d.holdSeconds>restPauseHoldMaximum(report()))) {
        message_=restPauseLimitError;return false;
    }
    if(dropTarget_) stopDrops("");
    const auto r=report();
    const int maximum=weightTicksDown(r,r.maxWeight);
    const int sign=d.step<0 ? -1 : 1;
    d.step = sign*std::min(weightTicksNearest(r,std::abs(d.step)),maximum);
    d.drops = std::clamp(d.drops, 0, 9); d.percent = std::clamp(d.percent, 0, 80);
    d.holdSeconds = std::clamp(d.holdSeconds, 1, 9);
    dropSettings_[selected_] = d; resetDrops();
    if(!d.enabled) dropStart_.reset();
    message_.clear();return true;
}
void TrainingApplication::toggleDrops() { auto d = drops(); d.enabled = !d.enabled; applyDrops(d); }
void TrainingApplication::resetDrops() {
    dropsCancelled_ = dropsExhausted_ = false;
    running_ = movementSeen_ = holdConsumed_ = wasLoaded_ = false;
    holdSince_ = telemetryRevision_ = 0; lastRep_ = 0; dropsCompleted_ = 0; dropTarget_.reset();
    repRevision_=dropSetEndRevision_=0;waitingForDropSet_=false;
    repSet_.reset();holdPosition_.reset();
}
void TrainingApplication::stopDrops(const char* reason) {
    if(*reason) recordControlFailure(reason);
    if(dropTarget_) {
        session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();dropTarget_.reset();
    }
    if(!selected_.empty()) dropSettings_[selected_].enabled=false;
    running_=false;dropsCancelled_=true;holdSince_=0;holdPosition_.reset();message_=reason;
}
void TrainingApplication::beginDropRestore(const TrainingReport& r) {
    if(!dropStart_) return;
    const auto start=*dropStart_;dropStart_.reset();
    // Only this observed workout's genuine rest/unload can initiate restoration.
    // It is never persisted, replayed after reconnect or caused by a segment close.
    if(!drops().enabled || r.unit!=start.unit || start.weight<r.minWeight) return;
    session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();
    resetDrops();dropsCancelled_=true;
    dropRestore_=DropRestore{start,now_,false};
    if(r.loaded) {
        const auto result=session_.setLoaded(false);sent(result);
        if(result!=TransportResult::Accepted) {dropRestore_.reset();message_="End-of-set unload failed; weight not restored";}
    }
}
void TrainingApplication::runDropRestore() {
    if(!dropRestore_) return;
    constexpr uint64_t restoreDeadlineMs=8000;
    if(now_-dropRestore_->at>restoreDeadlineMs || session_.commandFailed()) {
        session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();
        dropRestore_.reset();message_="Starting weight not restored; check VOLTRA";return;
    }
    if(pending_ || session_.busy() || !fresh()) return;
    const auto r=report();
    if(r.loaded || r.autoLoadArmed || r.unit!=dropRestore_->start.unit) {
        dropRestore_.reset();message_="State changed; starting weight not restored";return;
    }
    if(!r.liveWeightSupported) return; // Wait for settled unloaded control state.
    if(dropRestore_->weightSent) {
        const bool confirmed=r.settings.weight==dropRestore_->start.weight && message_.empty();
        dropRestore_.reset();message_=confirmed ? "" : "Starting weight not confirmed";return;
    }
    // Copy the current confirmed accessories. Restore only base, while unloaded,
    // and refuse an original weight that no longer fits the current limits.
    auto s=r.settings;s.weight=dropRestore_->start.weight;
    if(s.weight==r.settings.weight) {dropRestore_.reset();return;}
    if(!sendSettings(s)) {dropRestore_.reset();message_="Starting weight no longer available";return;}
    dropRestore_->weightSent=true;
}
void TrainingApplication::runDrops(const TrainingReport& r) {
    if(!drops().enabled) return;
    if(fresh() && drops().mode==DropMode::RestPause && drops().holdSeconds>restPauseHoldMaximum(r)) {
        // An external timer change must not silently shorten an armed hold or
        // permit a saved recipe to race Auto Unload. Require a new Apply.
        stopDrops(restPauseLimitError);return;
    }
    if(fresh() && !r.loaded) { resetDrops(); return; }
    if(fresh() && r.resting==true) {
        // A genuine device rest ends this set, unlike AA85 emitted by a base
        // edit while status stays active. No queued automatic change crosses it.
        if(dropTarget_) {session_.cancelTraining();pending_=false;settingsIntent_.reset();weightIntent_.reset();message_.clear();}
        const bool cancelled=dropsCancelled_;
        resetDrops();dropsCancelled_=cancelled;
        // Preserve the old completed counter only as a baseline while waiting
        // for the next real set's observed start. Loading need not toggle here.
        wasLoaded_=true;waitingForDropSet_=true;
        lastRep_=r.completedRep;repSet_=r.repSet;repRevision_=r.repRevision;
        telemetryRevision_=r.motionRevision;dropSetEndRevision_=r.setEndRevision;
        return;
    }
    // A confirmed floor/quota ends automation for this loaded sequence. Reps
    // still update independently; there is nothing left to validate or send.
    if(dropsExhausted_) return;
    if(!fresh() || !r.twinKnown || !r.liveWeightSupported || r.minWeight<1 || r.autoLoadArmed) {
        if(wasLoaded_ || dropTarget_) {
            const auto reason=!session_.status().empty() ? "Drops stopped: "+session_.status() : "Drops stopped: state unavailable";
            stopDrops(reason.c_str());
        }
        return;
    }
    if (dropsCancelled_) return; // A failed unload must never restart automation.
    const auto d = drops();
    if (dropTarget_ && !pending()) {
        if (session_.commandFailed() || !message_.empty() || r.settings.weight != *dropTarget_) {
            stopDrops("Drop unconfirmed; stopped");return;
        }
        diagnostic("Drop base readback ticks="+std::to_string(*dropTarget_)+" forceTicks="+
            (r.force ? std::to_string(*r.force) : "unknown")+"; resistance effect not verified");
        confirmedDropWeight_=*dropTarget_;++dropConfirmationRevision_;
        dropTarget_.reset(); ++dropsCompleted_;
    }
    const bool descending=d.mode==DropMode::RestPause ? d.percent>0 : d.step<0;
    if(!dropTarget_ && !pending() && ((descending && r.settings.weight<=r.minWeight) ||
       (d.mode==DropMode::RestPause && dropsCompleted_>=d.drops))) {
        dropsExhausted_=true;running_=false;holdSince_=0;holdPosition_.reset();return;
    }
    if (!wasLoaded_) {
        wasLoaded_=true;lastRep_=r.completedRep;repSet_=r.repSet;repRevision_=r.repRevision;
        telemetryRevision_=r.motionRevision;dropSetEndRevision_=r.setEndRevision;
        waitingForDropSet_=r.setActive==false;
    }
    constexpr uint64_t repFreshMs=2000;
    const bool motionFresh=r.motionRevision && now_>=r.motionAtMs && now_-r.motionAtMs<=motionFreshMs;
    if(r.setEndRevision!=dropSetEndRevision_) {
        dropSetEndRevision_=r.setEndRevision;waitingForDropSet_=true;
    }
    if(waitingForDropSet_) {
        // Black can finish a set without unloading. Its completed-rep report
        // still belongs to that old set until the next rep finishes. Rebase
        // only after observing the new set's actual zero-count moving stream;
        // a new set number alone must not excuse a gap or replay missed drops.
        running_=movementSeen_=holdConsumed_=false;holdSince_=0;holdPosition_.reset();
        if(motionFresh && r.setActive==true && r.moving && (r.rep==0 || d.mode==DropMode::RestPause) &&
           r.setNumber && r.setNumber!=repSet_) {
            // A base change can close Black's statistics set before readback
            // finishes. Observe the next start now; the pending-drop check
            // below still requires confirmation before another completed rep
            // can trigger a write. Never reset RP's quota on device set IDs:
            // it belongs to the armed loaded sequence, not a weight segment.
            repSet_=r.setNumber;lastRep_=0;waitingForDropSet_=false;
        }
    }
    if(!motionFresh) {
        if(d.mode==DropMode::RestPause) {
            // A short stream gap pauses RP instead of permanently disabling it.
            // Never accrue hold time through the gap. Require fresh movement
            // and a whole new hold; a prolonged loss still disarms automation.
            constexpr uint64_t rpMotionLossStopMs=2000;
            movementSeen_=false;holdConsumed_=true;holdSince_=0;holdPosition_.reset();
            if(r.motionRevision && (now_<r.motionAtMs || now_-r.motionAtMs>rpMotionLossStopMs))
                stopDrops("Drops stopped: motion reports lost");
        } else if(running_) stopDrops("Drops stopped: motion reports lost");
        return; // Wait for first actual sample after loading, never infer a hold.
    }
    running_=!waitingForDropSet_;
    const bool newRep=r.repRevision!=repRevision_;
    bool completed=false;
    if(newRep) {
        repRevision_=r.repRevision;
        if(d.mode==DropMode::Stairs && ((repSet_ && r.repSet!=repSet_) || r.completedRep<lastRep_ || r.completedRep>lastRep_+1 ||
           now_<r.repAtMs || now_-r.repAtMs>repFreshMs)) {
            stopDrops("Rep sequence changed; drops stopped");return;
        }
        completed=r.completedRep==lastRep_+1;
        repSet_=r.repSet;lastRep_=r.completedRep;
    }
    if (dropTarget_) {
        if(completed && d.mode==DropMode::Stairs) stopDrops("Rep before confirmation; drops stopped");
        return;
    }
    if(waitingForDropSet_) return;
    bool trigger = false;
    if (d.mode == DropMode::Stairs) {
        // A completed counter is not permission to change resistance later in
        // the rep. The adapter must still confirm the observed return endpoint.
        if(completed && d.step!=0 && (!r.completedRepWindow || r.repActive!=false || r.moving)) {
            stopDrops("Drop missed; Stairs stopped");return;
        }
        trigger=completed && d.step!=0;
    } else {
        if(r.motionRevision==telemetryRevision_) return;
        telemetryRevision_=r.motionRevision;
        if (r.moving) { movementSeen_ = true; holdConsumed_ = false; holdSince_ = 0;holdPosition_.reset(); }
        else if (r.held && movementSeen_ && !holdConsumed_) {
            // Cumulative drift must not look stationary just because each
            // individual sample moves less than the speed threshold.
            constexpr int holdDriftMm=10;
            if(!holdSince_ || (holdPosition_ && r.positionMm && std::abs(*holdPosition_-*r.positionMm)>holdDriftMm)) {
                holdSince_=r.motionAtMs;holdPosition_=r.positionMm;
            }
            trigger = r.motionAtMs - holdSince_ >= uint64_t(d.holdSeconds)*1000 && dropsCompleted_ < d.drops && d.percent > 0;
        } else {holdSince_ = 0;holdPosition_.reset();}
    }
    if (!trigger) return;
    if(pending()) {stopDrops("Busy at drop trigger; drops stopped");return;}
    auto s = r.settings;
    s.weight = d.mode == DropMode::Stairs ? nextStairWeight(r,d.step) : nextRestPauseWeight(r,d.percent);
    if (s.weight == r.settings.weight) return;
    // This dedicated session operation copies the confirmed accessories and
    // can emit only a base write. No manual modifier trimming or load command.
    const auto result=session_.setLiveWeight(s.weight);sent(result);
    if (result==TransportResult::Accepted) {
        settingsIntent_=s;dropTarget_ = s.weight;holdConsumed_=true;movementSeen_=false;
    } else {
        const auto reason=session_.status().empty() ? std::string("Drop request refused; stopped") : session_.status();
        stopDrops(reason.c_str());
    }
}
void TrainingApplication::tick(uint64_t now) {
    now_ = now;
    // Bound background radio work before polling can launch an internal retry.
    // A user selection has its own existing connection/retry policy.
    if(topologyProbeActive_ && !controller_.confirmed().connected &&
       (now_<topologyProbeStartedAt_ || now_-topologyProbeStartedAt_>=topologyConnectDeadlineMs)) stopTopologyProbe();
    // Cancel stale permissions before polling can dispatch a queued intent;
    // retain an established unload route while polling renews its live state.
    if(selectionPhase_==SelectionPhase::Ready && !retainSelectionControl(false)) return;
    session_.allowTwinWorkoutReentry(!maintenance_ && selectionPhase_==SelectionPhase::Ready && selectionResolvedTwin_);
    controller_.tick(now);
    if(topologyProbeActive_) {tickTopologyProbe();return;}
    refreshTwinDevices();
    if(runSelection()) return;
    if(connecting()) return;
    if(connectingDevice_ && !connected()) {
        const std::string feedback=controller_.feedback();
        message_=feedback=="Disconnected" && !session_.status().empty() ? session_.status() : feedback;
        if(!message_.empty()) {
            lastConnectionFailure_=("at="+std::to_string(now_)+" reason="+message_).substr(0,180);
            diagnostic("Connection failure: "+message_);
        }
    }
    else if(connected() && message_.compare(0,14,"Connecting to ")==0) message_.clear();
    if (scanPending_ && !transport_.scanInProgress()) { scanPending_=false; refreshDiscovery(transport_.discovered()); }
    if (!scanned_ || now - scannedAt_ >= discoveryIntervalMs) scan();
    if (!connected()) {
        // Keep the transport's failed-attempt explanation. A protocol session
        // that never opened only knows "Connection lost" and would erase it.
        if(!connectingDevice_ && !selected_.empty() && !session_.status().empty()) message_=session_.status();
        restSince_.reset();restSetRevision_=0;
        dropProgress_={};
        confirmedDropWeight_.reset();
        dropStart_.reset();dropRestore_.reset();
        if(!selected_.empty()) dropSettings_[selected_].enabled=false;
        connectingDevice_.reset(); presetIntent_.reset(); settingsIntent_.reset(); observedLoaded_ = false; pending_ = requestedLoad_ = autoLoadRequested_ = false; weightIntent_.reset(); resetDrops();
        startTopologyProbe();return;
    }
    if (connectingDevice_ && session_.accepted()) rememberConnection();
    const auto r = report();
    observeTwin(r);refreshTwinDevices();
    // A physical pair may be identified for presentation while commands remain
    // quarantined. No automatic recipe/restore executor runs in that view.
    if(selectionReadOnlyView_ || (selectionResolvedTwin_ && (!twinPermissionFresh() || !fresh() || !readbackFresh()))) return;
    if(!fresh()) restSince_.reset();
    else {
        const bool wasDropActive=dropProgress_.active;
        dropProgress_.observe(r,drops().enabled,now_);
        if(!wasDropActive && dropProgress_.active) {
            startingWeight_=r.settings.weight;
            if(!dropRestore_) dropStart_=DropStart{r.settings.weight,r.unit};
        }
        if(dropProgress_.visible) {
            // Ignore statistics closes caused by changing base. Only a device
            // rest phase or confirmed unload starts the grouped rest clock.
            restSetRevision_=r.setEndRevision;
            if(dropProgress_.active && r.setActive!=false) restSince_.reset();
            else if((wasDropActive || dropProgress_.active) && !restSince_) restSince_=now_;
        } else {
            // User-approved fallback (Oct 24): only an observed device set-close
            // starts rest. Taps, load toggles, reconnect and settings never do.
            if(r.setEndRevision>restSetRevision_) {
                restSetRevision_=r.setEndRevision;
                if(r.setEndedAtMs<=now_) restSince_=r.setEndedAtMs;
            }
            // New actual movement in an active set ends the preceding rest period.
            if(restSince_ && r.loaded && r.setActive==true && r.motionRevision && r.motionAtMs>=*restSince_ && r.moving)
                restSince_.reset();
        }
        if(wasDropActive && !dropProgress_.active && dropProgress_.reps>0) beginDropRestore(r);
    }
    // Sessions own their bounded transaction deadlines. Do not clear intent
    // mid-transaction, and do not turn an unrelated report into a successful ACK.
    if (pending_ && !session_.busy() && (session_.commandFailed() || r.revision > sentRevision_ || now - sentAt_ >= 3000)) {
        message_ = session_.commandFailed() ? session_.status() : r.revision > sentRevision_ ? "" : "No confirmation";
        const auto matches=[](Modifier a,Modifier b) {return a.enabled==b.enabled && (!a.enabled || sameModifier(a,b));};
        if(!session_.commandFailed() && settingsIntent_ && (r.revision<=sentRevision_ || !fresh() ||
           r.settings.weight!=settingsIntent_->weight || r.settings.magicMode!=settingsIntent_->magicMode ||
           !matches(r.settings.magic,settingsIntent_->magic) || !matches(r.settings.eccentric,settingsIntent_->eccentric)))
            message_="Settings not confirmed";
        if(!message_.empty()) {
            recordControlFailure(message_);
            if(requestedLoad_) {loadRefusalMessage_=message_;++loadRefusalRevision_;}
        }
        pending_ = requestedLoad_ = autoLoadRequested_ = false; weightIntent_.reset(); settingsIntent_.reset();
        if (presetIntent_) {
            auto actual = currentPreset(); actual.drops = presetIntent_->drops;
            if (!session_.commandFailed() && r.revision > sentRevision_ && fresh() && !r.loaded &&
                envelopeHundredths(r.settings) <= r.maxWeight*100 && samePreset(actual, *presetIntent_)) applyDrops(presetIntent_->drops);
            else message_ = "Preset not confirmed";
            presetIntent_.reset();
        }
    }
    constexpr uint64_t pendingHintDelayMs = 250;
    if (pending_ && !session_.unloading() && now-sentAt_ >= pendingHintDelayMs && message_.empty()) message_ = "Awaiting device";
    if (fresh() && r.loaded && !observedLoaded_) {
        startingWeight_ = r.settings.weight;
        if(!pending_) message_.clear(); // A confirmed external load resolves a prior load refusal.
    }
    if (fresh()) observedLoaded_ = r.loaded;
    runDropRestore();
    if(!dropRestore_) runDrops(r);
}
}
