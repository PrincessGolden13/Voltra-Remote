#pragma once
namespace voltra {
// Source evidence: docs/PROTOCOL_NOTES.md. Discover UUIDs, never fixed ATT handles.
struct VoltraGattProfile {
    static constexpr const char* service = "e4dada34-0867-8783-9f70-2ca29216c7e4";
    static constexpr const char* write = "a010891d-f50f-44f0-901f-9a2421a9e050";
    static constexpr const char* notify = "55ca1e52-7354-25de-6afc-b7df1e8816ac";
    // Third notification channel in the official-app capture and Android/HA.
    static constexpr const char* telemetry = "ca94658c-0525-5046-e78b-5391b65f47ad";
};
}
