#pragma once
#include <esp_core_dump.h>
#include <esp_partition.h>
#include <soc/soc_memory_types.h>
#include <array>
#include <cstring>
#include <optional>
#include <string>

namespace voltra {
namespace {
// ESP-IDF 4.4.7's ELF core_dump_elf.c and Xtensa core_dump_port.c define
// these layouts. Read only the fields needed for a consented crash summary.
// Reject other formats rather than interpreting arbitrary partition bytes.
constexpr uint32_t dumpHeaderBytes=20, dumpCrcBytes=4, dumpFormat=0x00090100;
constexpr uint32_t elfHeaderBytes=52, programHeaderBytes=32, noteHeaderBytes=12;
constexpr uint32_t elfCore=4, elfXtensa=94, programNote=4, elfAppNote=8266, extraNote=677, processStatusNote=1;
constexpr uint32_t appNoteBytes=72, extraNoteBytes=148, statusNoteBytes=588, exceptionCauseRegister=232;
constexpr uint32_t statusPidOffset=24, statusPcOffset=72;
constexpr uint32_t maximumProgramHeaders=140, maximumNotes=128;
constexpr size_t fingerprintCharacters=16;

uint32_t littleWord(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1])<<8 | uint32_t(p[2])<<16 | uint32_t(p[3])<<24;
}
uint16_t littleHalf(const uint8_t* p) {return uint16_t(p[0]) | uint16_t(p[1])<<8;}

struct CrashNotes {
    std::string fingerprint;
    std::optional<uint32_t> cause, pc;
};

CrashNotes readCrashNotes() {
    const auto* partition=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_COREDUMP,nullptr);
    size_t address=0,size=0;
    if(!partition || esp_core_dump_image_get(&address,&size)!=ESP_OK || address!=partition->address ||
        size<dumpHeaderBytes+elfHeaderBytes+dumpCrcBytes || size>partition->size) return {};
    std::array<uint8_t,dumpHeaderBytes+elfHeaderBytes> header{};
    if(esp_partition_read(partition,0,header.data(),header.size())!=ESP_OK || littleWord(header.data())!=size ||
        littleWord(header.data()+4)!=dumpFormat) return {};

    const auto* elf=header.data()+dumpHeaderBytes;
    constexpr uint8_t ident[]={0x7f,'E','L','F',1,1,1}; // ELF32, little-endian, version 1.
    const size_t elfSize=size-dumpHeaderBytes-dumpCrcBytes;
    const uint32_t programTable=littleWord(elf+28);
    const uint16_t programCount=littleHalf(elf+44);
    if(std::memcmp(elf,ident,sizeof(ident))!=0 || littleHalf(elf+16)!=elfCore || littleHalf(elf+18)!=elfXtensa ||
        littleHalf(elf+40)!=elfHeaderBytes || littleHalf(elf+42)!=programHeaderBytes || programTable!=elfHeaderBytes ||
        programCount>maximumProgramHeaders || programCount>(elfSize-programTable)/programHeaderBytes) return {};
    const auto read=[&](size_t offset,uint8_t* data,size_t length) {
        return offset<=elfSize && length<=elfSize-offset &&
            esp_partition_read(partition,dumpHeaderBytes+offset,data,length)==ESP_OK;
    };
    CrashNotes notes;
    uint32_t crashedTask=0, firstTask=0, firstPc=0;
    unsigned count=0;
    for(uint16_t i=0;i<programCount;++i) {
        std::array<uint8_t,programHeaderBytes> program{};
        if(!read(programTable+i*programHeaderBytes,program.data(),program.size())) return {};
        if(littleWord(program.data())!=programNote) continue;
        size_t offset=littleWord(program.data()+4);
        const size_t bytes=littleWord(program.data()+16);
        if(bytes!=littleWord(program.data()+20) || offset>elfSize || bytes>elfSize-offset) return {};
        const size_t end=offset+bytes;
        while(offset<end) {
            std::array<uint8_t,noteHeaderBytes> note{};
            if(++count>maximumNotes || end-offset<note.size() || !read(offset,note.data(),note.size())) return {};
            offset+=note.size();
            const size_t nameBytes=littleWord(note.data());
            const size_t dataBytes=littleWord(note.data()+4);
            const auto type=littleWord(note.data()+8);
            std::array<uint8_t,32> name{};
            if(nameBytes==0 || nameBytes>name.size() || nameBytes%4 || nameBytes>end-offset ||
                !read(offset,name.data(),nameBytes)) return {};
            offset+=nameBytes;
            const size_t padding=(4-dataBytes%4)%4;
            if(dataBytes>end-offset || padding>end-offset-dataBytes) return {};
            if(type==elfAppNote && nameBytes==20 && dataBytes==appNoteBytes &&
                std::memcmp(name.data(),"ESP_CORE_DUMP_INFO",19)==0) {
                std::array<uint8_t,4+fingerprintCharacters+1> identity{};
                if(!read(offset,identity.data(),identity.size()) || littleWord(identity.data())!=dumpFormat) return {};
                if(identity.back()!=0) return {};
                for(size_t digit=4;digit<identity.size()-1;++digit) {
                    const auto c=identity[digit];
                    if(!((c>='0' && c<='9') || (c>='a' && c<='f'))) return {};
                    notes.fingerprint+=char(c);
                }
            } else if(type==extraNote && nameBytes==12 && dataBytes==extraNoteBytes &&
                std::memcmp(name.data(),"EXTRA_INFO",11)==0) {
                std::array<uint8_t,12> extra{};
                if(!read(offset,extra.data(),extra.size())) return {};
                crashedTask=littleWord(extra.data());
                const auto cause=littleWord(extra.data()+8);
                if(littleWord(extra.data()+4)==exceptionCauseRegister && cause<=255) notes.cause=cause;
            } else if(type==processStatusNote && nameBytes==8 && dataBytes==statusNoteBytes && !firstTask &&
                std::memcmp(name.data(),"CORE",5)==0) {
                // Verify the first register note belongs to the crashed task.
                // The task identifier and every register other than PC stay local.
                std::array<uint8_t,4> field{};
                if(!read(offset+statusPidOffset,field.data(),field.size())) return {};
                firstTask=littleWord(field.data());
                if(!read(offset+statusPcOffset,field.data(),field.size())) return {};
                firstPc=littleWord(field.data());
            }
            offset+=dataBytes+padding;
        }
    }
    if(notes.fingerprint.size()!=fingerprintCharacters || !crashedTask) return {};
    if(firstTask==crashedTask && esp_ptr_executable(reinterpret_cast<const void*>(firstPc))) notes.pc=firstPc;
    return notes;
}
}
}
