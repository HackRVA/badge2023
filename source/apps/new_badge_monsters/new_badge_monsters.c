#include <stdint.h>
#include <string.h>

#include "new_badge_monsters.h"
#include "new_badge_monsters_assets.h"
#include "new_badge_monsters_ir.h"
#include "audio.h"
#include "badge.h"
#include "button.h"
#include "colors.h"
#include "framebuffer.h"
#include "key_value_storage.h"
#include "utils.h"

#define ARRAYSIZE(x) (sizeof((x)) / sizeof((x)[0]))

#ifdef __linux__
#include <stdio.h>
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

/***************************************** TYPES *************************************************/
enum app_states {
    APP_INIT,
    GAME_MENU,
    MONSTER_MENU,
    SHOW_MONSTER,
    SHOW_DESCRIPTION,
    TRADE_MONSTERS,
    EXIT_APP
#ifdef __linux__
    , ENABLE_ALL_MONSTERS //7 for testing, grants ownership of all monsters
#endif
};

struct new_monster
{
    char name[DYNMENU_MAX_TITLE];
    bool owned;
    int color;
    char blurb[128];
    const struct asset *asset;
};

enum menu_level_t {
    GAME_MENU_LEVEL,
    MONSTER_MENU_LEVEL
};

// each game state is mapped to a function with this type signature
typedef void (*state_to_function_map_fn_type)(void);

/***************************************** PROTOTYPES ********************************************/
/********* STATE FUNCTIONS ****************************/
// code should not call these directly; instead set app_state
static void app_init(void);
static void game_menu(void);
static void monster_menu(void);
static void show_monster(void);
static void show_description(void);
static void trade_monsters(void);
static void exit_app(void);

/********* INPUT HANDLERS **************************/
static void game_menu_button_handler(void);
static void monster_menu_button_handler(void);
static void show_monster_button_handler(void);
static void show_description_button_handler(void);
static void trade_monsters_button_handler(void);

/******** SAVE & RESTORE ***************************/
char *key_from_monster(const struct new_monster *m, char *key, size_t len);
static void set_monster_owned(const int monster_id, const bool owned);
static void load_from_flash(void);
static void save_to_flash(void);

/******** ENABLE ***********************************/
// void enable_monster(const int monster_id);
#ifdef __linux__
static void enable_all_monsters(void);
#endif

/******** TRADING **********************************/
static void trade_monsters(void);

/******** MENUS ************************************/

static void change_menu_level(enum menu_level_t level);
static void draw_menu(void);
static void setup_monster_menu(void);
static void setup_main_menu(void);

#ifdef __linux__
static const char *menu_level_str(const enum menu_level_t menu_level);
#endif


/***************************************** GLOBALS ***********************************************/
struct new_monster new_monsters[] = {
    {
        "cryptoraptor",
        true,
        RED,
        "This one only roars in cryptocurrency",
        &new_badge_monsters_assets_Crypto_Raptor
    }
  , {
        "2FactorTiger",
        false,
        RED,
        "Double the security, double the fun",
        &new_badge_monsters_assets_Two_Factor_Tiger
    }
  , {
        "firewallFlyer",
        false,
        RED,
        "Can't touch this, unless you're an allowed IP",
        &new_badge_monsters_assets_Firewall_Flyer
    }
  , {
        "trojanTurtle",
        false,
        RED,
        "Slow and sneaky, never underestimate it",
        &new_badge_monsters_assets_Trojan_Turtle
    }
  , {
        "phishinPhoenix",
        false,
        RED,
        "Spams your inbox, then rises from its ashes",
        &new_badge_monsters_assets_Phishing_Phoenix
    }
  , {
        "hackerHawk",
        false,
        RED,
        "Always watching from above, waiting to swoop in",
        &new_badge_monsters_assets_Hacker_Hawk
    }
  , {
        "malwareMantis",
        false,
        RED,
        "Prays on your system's vulnerabilities",
        &new_badge_monsters_assets_Malware_Mantis
    }
  , {
        "ddosDragon",
        false,
        RED,
        "Breathes a fire of traffic at your servers",
        &new_badge_monsters_assets_DDOS_Dragon
    }
  , {
        "keylogKoala",
        false,
        RED,
        "It's not eucalyptus leaves it's after",
        &new_badge_monsters_assets_Keylogger_Koala
    }
  , {
        "wormWombat",
        false,
        RED,
        "Burrows deep into your system",
        &new_badge_monsters_assets_Worm_Wombat
    }
  , {
        "adwareAnteater",
        false,
        RED,
        "Feeds on your browsing habits",
        &new_badge_monsters_assets_Adware_Anteater
    }
  , {
        "rootkitRhino",
        false,
        RED,
        "Charges at your system's core",
        &new_badge_monsters_assets_Rootkit_Rhino
    }
  , {
        "botnetBat",
        false,
        RED,
        "Flies in the darkness of the web",
        &new_badge_monsters_assets_Botnet_Bat
    }
  , {
        "dnsDolphin",
        false,
        RED,
        "Loves to play redirect games",
        &new_badge_monsters_assets_DNS_Dolphin
    }
  , {
        "sslShark",
        false,
        RED,
        "Swims in a sea of encrypted data",
        &new_badge_monsters_assets_SSL_Shark
    }
  , {
        "spamSpider",
        false,
        RED,
        "Weaves a web of unwanted emails",
        &new_badge_monsters_assets_Spam_Spider
    }
  , {
        "ransomRabbit",
        false,
        RED,
        "Hops into your files, then locks them up",
        &new_badge_monsters_assets_Ransomware_Rabbit
    }
  , {
        "snifferSnail",
        false,
        RED,
        "Slow but can smell your data from miles away",
        &new_badge_monsters_assets_Sniffer_Snail
    }
  , {
        "backdoorBee",
        false,
        RED,
        "Buzzes into your system through the back",
        &new_badge_monsters_assets_Backdoor_Bee
    }
  , {
        "exploitEagle",
        false,
        RED,
        "Soars high to find unpatched vulnerabilities",
        &new_badge_monsters_assets_Exploit_Eagle
    }
  , {
        "bruFrcBaboon",
        false,
        RED,
        "Not subtle, but sometimes it works",
        &new_badge_monsters_assets_Brute_Force_Baboon
    }
  , {
        "socNgnrSqrl",
        false,
        RED,
        "Collects your info like acorns",
        &new_badge_monsters_assets_Social_Engineer_Squirrel
    }
  , {
        "packSnifPuma",
        false,
        RED,
        "Stealthily stalks your network traffic",
        &new_badge_monsters_assets_Packet_Sniffer_Puma
    }
  , {
        "proxyPorcupn",
        false,
        RED,
        "Its spikes are like multiple IP addresses",
        &new_badge_monsters_assets_Proxy_Porcupine
    }
  , {
        "intDetIguana",
        false,
        RED,
        "Keeps a cold-blooded watch on your network",
        &new_badge_monsters_assets_Intrusion_Detection_Iguana
    }
  , {
        "penTestPengo",
        false,
        RED,
        "Slides into your defenses with ease",
        &new_badge_monsters_assets_PenTest_Penguin
    }
  , {
        "axsCtrlGator",
        false,
        RED,
        "Keeps unauthorized users at bay, snappily",
        &new_badge_monsters_assets_Access_Control_Alligator
    }
  , {
        "idsImpalas",
        false,
        RED,
        "Fast and efficient at detecting intrusions",
        &new_badge_monsters_assets_IDS_Impalas
    }
  , {
        "siemSloth",
        false,
        RED,
        "Slow but steady wins the security race",
        &new_badge_monsters_assets_SIEM_Sloth
    }
  , {
        "vpnVulture",
        false,
        RED,
        "Flies over geo-restrictions with ease",
        &new_badge_monsters_assets_VPN_Vulture
    }
};

// type, but it must be declared AFTER new_monsters for ARRAYSIZE
/*
 * global state for game -- used by almost all functions.
*/
struct game_state {
    // used in render_screen to skip render if unchanged
    bool screen_changed;
    unsigned int current_monster;
    unsigned int nmonsters;
    // TODO: (alf) looks like app_state and menu_level could be combined.
    // split state is just confusing.
    enum app_states app_state;
    bool trading_monsters_enabled;
    unsigned int initial_mon;
    struct dynmenu menu;
    struct dynmenu_item menu_item[ARRAYSIZE(new_monsters) + 1];
    // need to remember menu level even in non-menu states
    enum menu_level_t menu_level;
};


static struct game_state state = {
    false, 0, 0, APP_INIT, false,
    0, {
        "Badge Monsters",
        "",
        "",
        NULL,
        0,
        8,
        0,
        0,
        0,
        0xFFFFFF,
        0x0
    }, {}, GAME_MENU_LEVEL
};

static state_to_function_map_fn_type state_to_function_map[] = {
    app_init,
    game_menu,
    monster_menu,
    show_monster,
    show_description,
    trade_monsters,
    exit_app,
#ifdef __linux__
    enable_all_monsters,
#endif
};

/***************************************** End of GLOBALS ****************************************/

void badge_monsters_cb(__attribute__((unused)) struct menu_t *m)
{
    state_to_function_map[state.app_state]();
}

/***************************************** STATE FUNCTIONS ***************************************/
/*
 * Each of these functions is called directly from the callback based on the current app state.
 * Each should do roughly the following:
 * 1. If screen_changed is false, draw to the screen based on the current state.
 * 2. check for user input
 * 3. set app_state to next state, if it's different
 */

/*
 * Initializes data structures and screen, loads any saved monsters, sets up main menu.
 * state --> GAME_MENU
 */
static void app_init()
{
    // graphics hardware init
    FbInit();
    // set up to receive IR
    register_ir_packet_callback(ir_packet_callback);
    // initialize menu and load main menu
    dynmenu_init(&state.menu, state.menu_item, ARRAYSIZE(new_monsters));
    setup_main_menu();
    state.menu_level = GAME_MENU_LEVEL;

    // set up monster data structures, load
    state.nmonsters = ARRAYSIZE(new_monsters);
    load_from_flash();
    // initial monster different for different badges
    state.initial_mon = badge_system_data()->badgeId % state.nmonsters;
    state.current_monster = state.initial_mon;
    enable_monster(state.initial_mon);

    // tell app to draw screen, go to menu
    state.screen_changed = true;
    state.app_state = GAME_MENU;
    LOG("app_init complete\n");
}

/*
 * Saves monsters, de-initializes data, return to badge menu
 */
static void exit_app(void)
{
    state.app_state = APP_INIT;
    save_to_flash();
    unregister_ir_packet_callback(ir_packet_callback);
    returnToMenus();
}

static void game_menu(void)
{
    if (state.screen_changed) {
        draw_menu();
        FbPushBuffer();
    }
    check_for_incoming_packets();
    game_menu_button_handler();
}

static void monster_menu(void)
{
    if (state.screen_changed) {
        draw_menu();
        FbPushBuffer();
    }
    check_for_incoming_packets();
    monster_menu_button_handler();
}

/*
 * If trading_monstes_enabled == false, sets it to true and draws a message to screen.
 * Sends an IR packet every 10 loops.
 */
static void trade_monsters(void)
{
    static int counter = 0;

    counter++;
    if ((counter % 10) == 0) { /* transmit our monster IR packet */
        build_and_send_packet(BADGE_IR_GAME_ADDRESS, BADGE_IR_BROADCAST_ID,
                              (OPCODE_XMIT_MONSTER << 12) | (state.initial_mon & 0x01ff));
        audio_out_beep(500, 100);
    }
    if (!state.trading_monsters_enabled) {
        FbClear();
        FbMove(10, 60);
        FbWriteLine("TRADING");
        FbMove(10, 70);
        FbWriteLine("MONSTERS!");
        FbPushBuffer();
        state.trading_monsters_enabled = true;
    }
    trade_monsters_button_handler();
}


/*
 * Displays state.current_monster on the screen.
 *
 * Postconditions: app_state = RENDER_SCREEN, screen_changed = true
 */
static void show_monster(void)
{
    const struct new_monster *monster = &new_monsters[state.current_monster];

    if (state.screen_changed) {
        LOG("render_monster: %s\n", monster->name);
        FbClear();
        FbMove(0, 10);
        FbImage4bit(monster->asset, 0);
        if(state.current_monster == state.initial_mon)
        {
            FbMove(0,10);
            FbWriteLine("--starting-mon--");
        }
        // TODO: approximately center based on name length
        FbMove(10,0);
        FbColor(monster->color);
        FbWriteLine(monster->name);

        FbColor(WHITE);
        FbMove(43, LCD_YSIZE - 15);
        FbWriteLine("|down|");
        FbColor(GREEN);

        FbMove(5, LCD_YSIZE - 15);
        FbWriteLine("<Back");


        FbMove(90, LCD_YSIZE - 15);
        FbWriteLine("desc>");

        FbSwapBuffers();
        FbPushBuffer();
        state.screen_changed = false;
    }
    show_monster_button_handler();
}

static void show_description(void)
{
    const char *desc = new_monsters[state.current_monster].blurb;
    if (state.screen_changed) {
        FbClear();
        FbColor(WHITE);

        FbMove(8, 5);
        FbWriteString(desc);
        FbMove(5, 120);
        FbWriteLine("<Back");
        FbPushBuffer();
    }
    state.screen_changed = false;
    show_description_button_handler();
}


/***************************************** INPUT HANDLERS ****************************************/
// app_init() and exit_app() don't need handlers

/*
 * Handles input for the main game menu:
 *
 * "up" --> go to previous menu item
 * "down" --> go to next menu item
 * "left"/"right" --> do nothing
 * "click" --> go to app state associated with menu item
 */
static void game_menu_button_handler(void) {
    const int down_latches = button_down_latches();
    const int rotary = button_get_rotation(0);

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
    {
        dynmenu_change_current_selection(&state.menu, -1);
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
    {
        dynmenu_change_current_selection(&state.menu, 1);
        state.screen_changed = true;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
    BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
    {
        const struct dynmenu_item menu_item = state.menu.item[state.menu.current_item];
        // menu item 0 is "monsters", and we need to change to other menu
        if (state.menu.current_item == 0) {
            change_menu_level(MONSTER_MENU_LEVEL);
            state.current_monster = state.menu.item[state.menu.current_item].cookie;
        }
        state.app_state = menu_item.next_state;
    }
}

/*
 * Handles input for the monster menu.
 *
 * "up" --> go to previous monster (and update current_monster)
 * "down" --> go to next monster (and update current_monster)
 * "left" --> change to MAIN_MENU level
 * "right"/"click" --> display currently selected monster
 */
static void monster_menu_button_handler(void) {
    const int down_latches = button_down_latches();
    const int rotary = button_get_rotation(0);

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
    {
        dynmenu_change_current_selection(&state.menu, -1);
        state.current_monster = state.menu.item[state.menu.current_item].cookie;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
    {
        dynmenu_change_current_selection(&state.menu, 1);
        state.current_monster = state.menu.item[state.menu.current_item].cookie;
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
    {
        change_menu_level(GAME_MENU_LEVEL);
        draw_menu();
    }
    else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
    {
        state.app_state = SHOW_MONSTER;
    }
    state.screen_changed = true; // make sure it gets redrawn
}

/*
 * On any button click, ends trading, returns to game menu
 */
static void trade_monsters_button_handler(void) {
    const int down_latches = button_down_latches();

    if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_ENCODER_2_SW, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
        BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
        state.trading_monsters_enabled = false;
        state.app_state = GAME_MENU;
        state.screen_changed = true;
    }
}

/*
 * Handles input for monster description screen
 *
 * "up" -->
 * "down" -->
 * "left" --> redisplay monster
 * "right"/"click" -->
 */
static void show_description_button_handler(void) {
    const int down_latches = button_down_latches();

    if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
             BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
    {
        state.app_state = SHOW_MONSTER;
        state.screen_changed = true;
    }
}

static void show_monster_button_handler(void) {
    const int down_latches = button_down_latches();
    const int rotary = button_get_rotation(0);
}

/***************************************** SAVE/RESTORE ******************************************/

char *key_from_monster(const struct new_monster *m, char *key, size_t len)
{
    snprintf(key, len, "monster/%s", m->name);

    return key;
}

static void load_from_flash()
{
    char key[20];
    int unused; //??
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        if (!flash_kv_get_int(key_from_monster(m, key, sizeof(key)), &unused)) {
            m->owned = false;
        }
    }
}

static void save_if_enabled(const struct new_monster *m)
{
    char key[20];

    if (m->owned) {
        flash_kv_store_int(key_from_monster(m, key, sizeof(key)), m->owned);
    }
}

static void save_to_flash(void)
{
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        save_if_enabled(m);
    }
}

/***************************************** MONSTER STATUS ****************************************/
static void set_monster_owned(const int monster_id, const bool owned)
{
    new_monsters[monster_id].owned = owned;
}

/*
 * Marks the monster with id `monster_id` as owned by user, beeps.
 */
void enable_monster(int monster_id)
{
    if(monster_id < 0 || (size_t) monster_id > ARRAYSIZE(new_monsters) - 1) {
        LOG("enable_monster: index %d is out of bounds!\n", monster_id);
        return;
    }
    set_monster_owned(monster_id, true);
    audio_out_beep(900, 200);
    LOG("enabled monster: %d\n", monster_id);
}

#ifdef __linux__
static void enable_all_monsters(void)
{
    for (size_t i = 0; i < ARRAYSIZE(new_monsters); i++)
        enable_monster(i);
    FbClear();
    FbMove(2, 2);
    FbWriteString("ALL MONSTERS\nENABLED!");
    state.app_state = GAME_MENU;
}
#endif


/***************************************** MENUS *************************************************/

/*
 * Draws the menu from the current state. If menu is main menu, add a line with counts
 * of unlocked and total monsters.
 */
static void draw_menu(void)
{
    dynmenu_draw(&state.menu);
    if(state.menu_level == MONSTER_MENU_LEVEL){
        int nunlocked = 0;
        char available_monsters[3];
        char unlocked_monsters[3];
        for(size_t i = 0; i < ARRAYSIZE(new_monsters); i++)
        {
            if(new_monsters[i].owned)
            {
                nunlocked++;
            }
        }

        snprintf( available_monsters, sizeof(available_monsters), "%d", (int)ARRAYSIZE(new_monsters));
        snprintf( unlocked_monsters, sizeof(unlocked_monsters), "%d", nunlocked);

        FbMove(1,25);
        FbWriteLine("Collected: ");
        FbWriteLine(unlocked_monsters);
        FbWriteLine("/");
        FbWriteLine(available_monsters);
    }
}

/*
 * Changes the menu from the current level to `level`, runs setup function.
 */
static void change_menu_level(enum menu_level_t level)
{
    // remember the most recent monster selected
    static int which_monster = -1;

    if (which_monster == -1)
        which_monster = state.initial_mon;

    if (strcmp(state.menu.title, "Monsters") == 0)
        which_monster = state.menu.current_item;

    if (level == GAME_MENU_LEVEL) {
        setup_main_menu();
    } else {
        setup_monster_menu();
        // set current monster to same as previous, if it's in menu
        if (state.menu.max_items > which_monster) {
            state.menu.current_item = which_monster; /* Stay on the same monster */
            state.current_monster = state.menu.item[state.menu.current_item].cookie;
        }
    }
    state.screen_changed = true;
    state.menu_level = level;
    LOG("change_menu_level: new level = %s\n", menu_level_str(state.menu_level));
}


#ifdef __linux__
static const char *menu_level_str(const enum menu_level_t menu_level) {
    // TODO: convert all "main" references to "game"
    return menu_level == GAME_MENU_LEVEL ? "MAIN" : "MONSTER_MENU";
}
#endif

/*
 * Builds a monster menu by clearing the menu, setting the title to "Monsters", and
 * for each monster which is owned, adding an entry with the monster's name, the next
 * state SHOW_MONSTER, and a cookie set to the index of the monster in the new_monsters
 * array.
 * If the current monster is shown in the menu (?), set the current item to the current
 * monster, otherwise set the current monster to the current menu item.
 */
static void setup_monster_menu(void)
{
    dynmenu_clear(&state.menu);
    state.menu.menu_active = 0;
    strncpy(state.menu.title, "Monsters", sizeof(state.menu.title));

    int index = 0;
    for(const struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++){
        if(m->owned)
            dynmenu_add_item(&state.menu, (char *)m->name, SHOW_MONSTER, index);
        index++;
    }
    dynmenu_add_item(&state.menu, "EXIT", GAME_MENU, 0);
    // I'm not sure this is accomplishing what it's supposed to
    if (state.current_monster < state.menu.max_items)
        state.menu.current_item = state.current_monster;
    else
        state.current_monster = state.menu.current_item;
    state.screen_changed = true;
}

/*
 * Builds the game main menu. Note that this and monster menu share the same menu
 * structure, it gets rebuilt when the menu_level changes.
 */
static void setup_main_menu(void)
{
    dynmenu_clear(&state.menu);
    snprintf(state.menu.title, sizeof(state.menu.title), "Badge Monsters");
    // OK, monsters menu needs its own state
    dynmenu_add_item(&state.menu, "Monsters", GAME_MENU, 0);
    dynmenu_add_item(&state.menu, "Trade Monsters", TRADE_MONSTERS, 1);
    dynmenu_add_item(&state.menu, "EXIT", EXIT_APP, 2);
#ifdef __linux__
    /* For testing purposes allow enabling all monsters on linux */
    dynmenu_add_item(&state.menu, "Test Monsters", ENABLE_ALL_MONSTERS, 3);
#endif
    state.screen_changed = true;
}

//**************************** DISPLAY ***********************************************

/*
 * Displays a monster, showing a new one each time it is called.
 */
void render_screen_save_monsters(void) {
    static unsigned char current_index = 0;
    current_index++;
    // wrap-around if out of bounds
    current_index %= ARRAYSIZE(new_monsters) - 1;

    FbClear();
    FbColor(BLACK);
    FbImage4bit(new_monsters[current_index].asset, 0);
}

