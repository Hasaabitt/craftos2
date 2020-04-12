/*
 * peripheral/drive.hpp
 * CraftOS-PC 2
 * 
 * This file defines the class for the drive peripheral.
 * 
 * This code is licensed under the MIT License.
 * Copyright (c) 2019-2020 JackMacWindows. 
 */

#ifndef PERIPHERAL_DRIVE_HPP
#define PERIPHERAL_DRIVE_HPP
#include <unordered_set>
#ifndef NO_MIXER
#include <SDL2/SDL_mixer.h>
#endif
#include "peripheral.hpp"
#include "../mounter.hpp"

enum disk_type {
    DISK_TYPE_NONE,
    DISK_TYPE_DISK,
    DISK_TYPE_AUDIO,
    DISK_TYPE_MOUNT
};

class drive: public peripheral {
private:
    disk_type diskType = DISK_TYPE_NONE;
    std::string mount_path;
    std::string path;
#ifndef NO_MIXER
    Mix_Music* music = NULL;
#else
    void* music = NULL;
#endif
    int id = 0;
    int isDiskPresent(lua_State *L);
    int getDiskLabel(lua_State *L);
    int setDiskLabel(lua_State *L);
    int hasData(lua_State *L);
    int getMountPath(lua_State *L);
    int hasAudio(lua_State *L);
    int getAudioTitle(lua_State *L);
    int playAudio(lua_State *L);
    int stopAudio(lua_State *L);
    int ejectDisk(lua_State *L);
    int getDiskID(lua_State *L);
    int insertDisk(lua_State *L, bool init = false);
public:
    static library_t methods;
    static peripheral * init(lua_State *L, const char * side) {return new drive(L, side);}
    static void deinit(peripheral * p) {delete (drive*)p;}
    destructor getDestructor() {return deinit;}
    library_t getMethods() { return methods; }
    drive(lua_State *L, const char * side);
    ~drive();
    int call(lua_State *L, const char * method);
    void update() {}
};

extern void driveInit();
extern void driveQuit();

#endif