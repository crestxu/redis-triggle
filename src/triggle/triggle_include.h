#ifndef TRIGGLE_INCLUDE
#define TRIGGLE_INCLUDE

#define BRIDGE_SYSTEM_CHANNEL "system"  //define the system default channel name to suscribe the system event
#define BRIDGE_DEFAULT_EVENT 1  //define default delete event when the keys expires
#define BRIDGE_KEY_NOTIFY 1
#define BRIDGE_KEY_UNNOTIFY 0 
#include"dict.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <ctype.h>
#include <math.h>
#define TRIGGLE_BEFORE 1
#define TRIGGLE_AFTER 0
#define BRIDGE_DEBUG 1 
struct redisClient;
struct redisObject;
struct dict;

enum BAIDU_BRIDGE_TRIGGLE{
	/*General keys operation*/
	DELETING_EVENT=203, //delete command triggle
    DELETED_EVENT,

    SETING_EVENT,
    SETED_EVENT,
	

    SETNXING_EVENT,
    SETNXED_EVENT,
	

    SETEXING_EVENT,
    SETEXED_EVENT,


	


    EXPIRING_EVENT,
    EXPIRED_EVENT,  //when expired the key from the database triggle
	/*String keys*/
	
    MSETING_EVENT,

    MSETED_EVENT,
	
    
    SETBITING_EVENT,

    SETBITED_EVENT,
	
    HDELING_EVENT,
    
    HDELED_EVENT,
    HSETING_EVENT,

    HSETED_EVENT,

    LPUSHING_EVENT,

    LPUSHED_EVENT,
    LPOPING_EVENT,
    LPOPED_EVENT,

    LREMING_EVENT,

    LREMED_EVENT,


    LSETING_EVENT,

    LSETED_EVENT,

    RPUSHING_EVENT,

    RPUSHED_EVENT,
	
    RPOPING_EVENT,
	
    
    
    SADDING_EVENT,
    
    SADDED_EVENT,
    SREMING_EVENT,

    SREMED_EVENT,
	/*ZSET*/
	ZADDING_EVENT,

	ZADDED_EVENT,

    ZREMING_EVENT,

    ZREMED_EVENT,
	/*PUB,SUB*/
    PUBLISH_EVENT,
    DELETE_EXPIRE
};


typedef struct bridge_db_triggle_t{
    enum BAIDU_BRIDGE_TRIGGLE event;
	struct redisObject * lua_scripts;
	int dbid;
}bridge_db_triggle_t;

typedef struct bridge_db_externtion_t{
	dict **triggle_scipts;
	int  *bridge_event;
    dict *triggle_cmds;
}bridge_db_externtion_t;


#define BRIDGE_DB_EXTENTIONS  struct bridge_db_externtion_t bridge_db;

#define CALL_BRIDGE_EVENT_BEFORE(arg) call_bridge_event_before(arg,-1)

#define CALL_BRIDGE_EVENT_AFTER(arg) call_bridge_event_after(arg,-1)
void init_bridge_server();

void do_bridge_notify(void *db,void *keyobj);

void triggleCommand(struct redisClient *c); //redisClient *c


void triggleDelCommand(struct redisClient *c); //redisClient *c


void triggleListCommand(struct redisClient *c); //redisClient *c


void call_bridge_event(struct redisClient *c,int place,int event_type);

void call_bridge_event_before(struct redisClient *c,int event_type);

void call_bridge_event_after(struct redisClient *c,int event_type);

void call_expire_delete_event(void *db,void *keyobj);


#endif
