//-----------------------------------------------------------------------------
// Copyright (C) 2013 m h swende <martin at swende.se>
//
// This code is licensed to you under the terms of the GNU GPL, version 2 or,
// at your option, any later version. See the LICENSE.txt file for the text of
// the license.
//-----------------------------------------------------------------------------
// Some lua scripting glue to proxmark core.
//-----------------------------------------------------------------------------

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "proxmark3.h"
#include "usb_cmd.h"
#include "cmdmain.h"
#include "scripting.h"
#include "util.h"
/**
 * The following params expected:
 *  UsbCommand c
 *@brief l_SendCommand
 * @param L
 * @return
 */
static int l_SendCommand(lua_State *L){

    /*
     *
     The SendCommand (native) expects the following structure:

     typedef struct {
      uint64_t cmd; //8 bytes
      uint64_t arg[3]; // 8*3 bytes = 24 bytes
      union {
        uint8_t  asBytes[USB_CMD_DATA_SIZE]; // 1 byte * 512 = 512 bytes (OR)
        uint32_t asDwords[USB_CMD_DATA_SIZE/4]; // 4 byte * 128 = 512 bytes
      } d;
    } PACKED UsbCommand;

    ==> A 544 byte buffer will do.
    **/
    //Pop cmd
    size_t size;
    const char *data = luaL_checklstring(L, 1, &size);
    if(size != sizeof(UsbCommand))
    {
        printf("Got data size %d, expected %d" , (int) size,(int) sizeof(UsbCommand));
        lua_pushstring(L,"Wrong data size");
        return 1;
    }

//    UsbCommand c = (*data);
    SendCommand(data);
    return 0; // no return values
}
/**
 * @brief The following params expected:
 * uint32_t cmd
 * size_t ms_timeout
 * @param L
 * @return
 */
static int l_WaitForResponseTimeout(lua_State *L){

    uint32_t cmd = 0;
    size_t ms_timeout = -1;

    //Check number of arguments
    int n = lua_gettop(L);
    if(n == 0)
    {
        //signal error by returning Nil, errorstring
        lua_pushnil(L);
        lua_pushstring(L,"You need to supply at least command to wait for");
        return 2; // two return values
    }
    if(n >= 1)
    {
        //pop cmd
        cmd = luaL_checkunsigned(L,1);
    }
    if(n >= 2)
    {
        //Did the user send a timeout ?
        //Check if the current top of stack is an integer
        ms_timeout = luaL_checkunsigned(L,2);
        //printf("Timeout set to %dms\n" , (int) ms_timeout);
    }

    UsbCommand response;

    if(WaitForResponseTimeout(cmd, &response, ms_timeout))
    {
        //Push it as a string
         lua_pushlstring(L,&response,sizeof(UsbCommand));

        return 1;// return 1 to signal one return value
    }else{
        //Push a Nil instead
        lua_pushnil(L);
        return 1;// one return value
    }
}
static int l_nonce2key(lua_State *L){ return CmdHF14AMfRdSc(luaL_checkstring(L, 1));}
static int l_PrintAndLog(lua_State *L){ return CmdHF14AMfDump(luaL_checkstring(L, 1));}
static int l_clearCommandBuffer(lua_State *L){
    clearCommandBuffer();
}
/**
 * @brief l_foobar is a dummy function to test lua-integration with
 * @param L
 * @return
 */
static int l_foobar(lua_State *L)
{
    //Check number of arguments
    int n = lua_gettop(L);
    printf("foobar called with %d arguments" , n);
    lua_settop(L, 0);
    printf("Arguments discarded, stack now contains %d elements", lua_gettop(L));
    UsbCommand response =  {CMD_MIFARE_READBL, {1337, 1338, 1339}};
    printf("Now returning a UsbCommand as a string");
    lua_pushlstring(L,&response,sizeof(UsbCommand));
    return 1;
}

/**
 * @brief Utility to check if a key has been pressed by the user. This method does not block.
 * @param L
 * @return boolean, true if kbhit, false otherwise.
 */
static int l_ukbhit(lua_State *L)
{
    lua_pushboolean(L,ukbhit() ? true : false);
    return 1;
}

int set_pm3_libraries(lua_State *L)
{
    static const luaL_Reg libs[] = {
        {"SendCommand",                 l_SendCommand},
        {"WaitForResponseTimeout",      l_WaitForResponseTimeout},
        {"nonce2key",                   l_nonce2key},
        {"PrintAndLog",                 l_PrintAndLog},
        {"foobar",                      l_foobar},
        {"ukbhit",                      l_ukbhit},
        {"clearCommandBuffer",          l_clearCommandBuffer},
        {NULL, NULL}
    };

    lua_pushglobaltable(L);
    // Core library is in this table. Contains '
    //this is 'pm3' table
    lua_newtable(L);

    //Put the function into the hash table.
    for (int i = 0; libs[i].name; i++) {
        lua_pushcfunction(L, libs[i].func);
        lua_setfield(L, -2, libs[i].name);//set the name, pop stack
    }
    //Name of 'core'
    lua_setfield(L, -2, "core");

    //-- remove the global environment table from the stack
    lua_pop(L, 1);
    return 1;
}