#include <stdint.h>
#include <string.h>
#ifdef __linux__
#include <stdio.h>
#endif

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

enum app_states {
    INIT_APP_STATE,
    GAME_MENU,
    RENDER_SCREEN,
    RENDER_MONSTER,
    TRADE_MONSTERS,
    CHECK_THE_BUTTONS,

    EXIT_APP,
    ENABLE_ALL_MONSTERS
};

/***************************************** GLOBALS ************************************************/

struct new_monster new_monsters[] = {
    {
        "2FactorTiger",
        false,
        RED,
        "Double the security, double the fun",
        &new_badge_monsters_assets_Two_Factor_Tiger
    }
  , {
        "cryptoraptor",
        false,
        RED,
        "This one only roars in cryptocurrency",
        &new_badge_monsters_assets_Crypto_Raptor
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

/*
 * global state for game -- used by almost all functions.
*/
struct game_state {
    bool screen_changed;
    int smiley_x;
    int smiley_y;
    unsigned int current_monster;
    unsigned int nmonsters;
    enum app_states app_state;
    bool trading_monsters_enabled;
    unsigned int initial_mon;
    struct dynmenu menu;
    struct dynmenu_item menu_item[ARRAYSIZE(new_monsters)];
    enum menu_level_t menu_level;
};

static struct game_state state = {
    false, 0, 0, 0, 0, INIT_APP_STATE, false,
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
    }, {}, MAIN_MENU
};

typedef void (*state_to_function_map_fn_type)(void);

static state_to_function_map_fn_type state_to_function_map[] = {
    app_init,
    game_menu,
    render_screen,
    render_monster,
    trade_monsters,
    check_the_buttons,
    exit_app,
#ifdef __linux__
    enable_all_monsters,
#endif
};

/***************************************** End of GLOBALS *****************************************/


void badge_monsters_cb(__attribute__((unused)) struct menu_t *m)
{
#ifdef __linux__
    // print only state changes, or we get 100s of useless lines of output
    static unsigned int last_state = EXIT_APP;
    if (last_state != state.app_state) {
        printf("badge_monsters_cb: state %d\n", state.app_state);
        last_state = state.app_state;
    }
#endif
    state_to_function_map[state.app_state]();
}

void app_init()
{
    FbInit();
    register_ir_packet_callback(ir_packet_callback);
#ifdef __linux__
    printf("app_init: %lu, %d\n", ARRAYSIZE(new_monsters), 0);
#endif
    dynmenu_init(&state.menu, state.menu_item, ARRAYSIZE(new_monsters));
#ifdef __linux__
    printf("app_init: adding %s to %p", new_monsters[0].name, &state.menu);
#endif
    setup_main_menu();
    dynmenu_add_item(&state.menu, new_monsters[0].name, RENDER_MONSTER, 0);
    change_menu_level(MAIN_MENU);
    state.app_state = GAME_MENU;
    state.screen_changed = true;
    state.smiley_x = LCD_XSIZE / 2; //?? name?
    state.smiley_y = LCD_XSIZE / 2;
    state.nmonsters = ARRAYSIZE(new_monsters);

    load_from_flash();
    state.initial_mon = badge_system_data()->badgeId % state.nmonsters;
    state.current_monster = state.initial_mon;
    enable_monster(state.initial_mon);
    state.app_state = GAME_MENU;
}

void exit_app(void)
{
    state.app_state = INIT_APP_STATE;
    save_to_flash();
    unregister_ir_packet_callback(ir_packet_callback);
    returnToMenus();
}

char *key_from_monster(const struct new_monster *m, char *key, size_t len)
{
    snprintf(key, len, "monster/%s", m->name);

    return key;
}

void load_from_flash()
{
    char key[20];
    int unused; //??
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        if (!flash_kv_get_int(key_from_monster(m, key, sizeof(key)), &unused)) {
            m->owned = false;
        }
    }
}

void save_if_enabled(const struct new_monster *m)
{
    char key[20];

    if (m->owned) {
        flash_kv_store_int(key_from_monster(m, key, sizeof(key)), m->owned);
    }
}

void save_to_flash(void)
{
    for (struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++) {
        save_if_enabled(m);
    }
}

void set_monster_owned(const int monster_id, const bool owned)
{
    new_monsters[monster_id].owned = owned;
}

void enable_monster(int monster_id)
{
    if(monster_id < 0 || (size_t) monster_id > ARRAYSIZE(new_monsters) - 1)
        return;

    new_monsters[monster_id].owned = true;
    audio_out_beep(900, 200);
    #ifdef __linux__
        printf("enabling monster: %d\n", monster_id);
    #endif
}

#ifdef __linux__
void enable_all_monsters(void)
{
    for (size_t i = 0; i < ARRAYSIZE(new_monsters); i++)
        enable_monster(i);
    FbClear();
    FbMove(2, 2);
    FbWriteString("ALL MONSTERS\nENABLED!");
    state.app_state = RENDER_SCREEN;
}
#endif


//********************************** MENUS *****************************************

void draw_menu(void)
{
    dynmenu_draw(&state.menu);
    if(state.menu_level != MONSTER_MENU){
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
    state.app_state = RENDER_SCREEN;
}

void change_menu_level(enum menu_level_t level)
{
    static int which_item = -1;

    if (which_item == -1)
        which_item = state.initial_mon;

    if (strcmp(state.menu.title, "Monsters") == 0)
        which_item = state.menu.current_item;

    switch(level){
        case MAIN_MENU:
            setup_main_menu();
            state.screen_changed = true;
            break;
        case MONSTER_MENU:
            setup_monster_menu();
            if (state.menu.max_items > which_item) {
        state.menu.current_item = which_item; /* Stay on the same monster */
                state.current_monster = state.menu.item[state.menu.current_item].cookie;
            }
            state.screen_changed = true;
            break;
        case DESCRIPTION:
            state.screen_changed = true;
            return;
    }
}

#ifdef __linux__
void print_menu_info()
{
    /* int next_state = menu.item[menu.current_item].next_state
       system("clear"); */
    printf("current item: %d\nmenu level: %d\ncurrent monster: %d\n menu item: %s\nn-menu-items: %d\ncookie_monster: %d\n",
           state.menu.current_item,
           state.menu_level,
           state.current_monster,
           state.menu.item[state.menu.current_item].text,
           state.menu.nitems,
           state.menu.item[state.menu.current_item].cookie);
}
#endif

void check_the_buttons(void)
{
    bool something_changed = false;
    int down_latches = button_down_latches();
    int rotary = button_get_rotation(0);

    /* If we are trading monsters, stop trading monsters on a button press */
    if (state.trading_monsters_enabled) {
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
            return;
        }
    }

    switch(state.menu_level){
        case MAIN_MENU:
            if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
            {
                dynmenu_change_current_selection(&state.menu, -1);
                state.screen_changed = true;
                something_changed = true;
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
            {
                dynmenu_change_current_selection(&state.menu, 1);
                state.screen_changed = true;
                something_changed = true;
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches))
            {
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
            {
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
            BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
            {
                switch(state.menu.current_item){
                    case 0:
                        change_menu_level(MONSTER_MENU);
                        state.current_monster = state.menu.item[state.menu.current_item].cookie;
                        break;
                    case 1:
                        state.app_state = TRADE_MONSTERS;
                        break;
                    case 2:
                        state.app_state = EXIT_APP;
                        break;
#ifdef __linux__
                    case 3:
                        state.app_state = ENABLE_ALL_MONSTERS;
                        break;
#endif
                }
            }

            break;
        case MONSTER_MENU:
            if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
            {
                dynmenu_change_current_selection(&state.menu, -1);
                state.screen_changed = true;
                state.current_monster = state.menu.item[state.menu.current_item].cookie;
                render_monster();
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
            {
                dynmenu_change_current_selection(&state.menu, 1);
                state.screen_changed = true;
                state.current_monster = state.menu.item[state.menu.current_item].cookie;
                render_monster();
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
            BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
            {
                change_menu_level(MAIN_MENU);
                something_changed = true;
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
            {
                show_message(new_monsters[state.current_monster].blurb);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
            BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
            {
                #ifdef __linux__
                    print_menu_info();
                #endif
                show_message(new_monsters[state.current_monster].blurb);
             }
            break;
        case DESCRIPTION:
            if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) || rotary < 0)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) || rotary > 0)
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
            BUTTON_PRESSED(BADGE_BUTTON_B, down_latches))
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches))
            {
                change_menu_level(MONSTER_MENU);
            }
            else if (BUTTON_PRESSED(BADGE_BUTTON_ENCODER_SW, down_latches) ||
            BUTTON_PRESSED(BADGE_BUTTON_A, down_latches))
            {
                change_menu_level(MONSTER_MENU);
            }
            break;
        default:
            break;
        }

    if (state.trading_monsters_enabled && !something_changed) {
        state.app_state = TRADE_MONSTERS;
    }

    if (something_changed && state.app_state == CHECK_THE_BUTTONS) {
        state.app_state = GAME_MENU;
    }
    return;
}

void setup_monster_menu(void)
{
    dynmenu_clear(&state.menu);
    state.menu.menu_active = 0;
    strncpy(state.menu.title, "Monsters", sizeof(state.menu.title));

    int index = 0;
    for(const struct new_monster *m = new_monsters; PART_OF_ARRAY(new_monsters, m); m++){
        if(m->owned)
            dynmenu_add_item(&state.menu, (char *)m->name, RENDER_MONSTER, index);
        index++;
    }
    if (state.current_monster < state.menu.max_items)
        state.menu.current_item = state.current_monster;
    else
        state.current_monster = state.menu.current_item;

    state.screen_changed = true;
    render_monster();
}

void setup_main_menu(void)
{
    dynmenu_clear(&state.menu);
    strncpy(state.menu.title, "Badge Monsters", sizeof(state.menu.title));
#ifdef __linux__
    printf("setup_main_menu\n");
#endif
    dynmenu_add_item(&state.menu, "Monsters", RENDER_SCREEN, 0);
    dynmenu_add_item(&state.menu, "Trade Monsters", TRADE_MONSTERS, 1);
    dynmenu_add_item(&state.menu, "EXIT", EXIT_APP, 2);
#ifdef __linux__
    /* For testing purposes allow enabling all monsters on linux */
    dynmenu_add_item(&state.menu, "Test Monsters", ENABLE_ALL_MONSTERS, 3);
#endif
    state.screen_changed = true;
}

void game_menu(void)
{
    state.menu_level = MAIN_MENU;
    draw_menu();
    check_for_incoming_packets();
    state.app_state = RENDER_SCREEN;
}

//**************************** DISPLAY ***********************************************

void render_monster(void)
{
    const struct new_monster *monster = &new_monsters[state.current_monster];

#ifdef __linux__
    printf("render\n");
    printf("render_monster: %s\n", monster->name);
#endif
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
    state.screen_changed = true;
    state.app_state = RENDER_SCREEN;
}

void show_message(const char *message)
{
    #ifdef __linux__
        printf("%s\n", message);
    #endif

    FbClear();
    FbColor(WHITE);

    FbMove(8, 5);
    FbWriteString(message);
    FbMove(5, 120);
    FbWriteLine("<Back");

    change_menu_level(DESCRIPTION);
    state.app_state = RENDER_SCREEN;
    state.screen_changed = true;
}

void render_screen(void)
{
    state.app_state = CHECK_THE_BUTTONS;
    if (!state.screen_changed)
        return;
    FbPushBuffer();
    state.screen_changed = false;
}

void render_screen_save_monsters(void) {
    static unsigned char current_index = 0;
    current_index++;
    // wrap-around if out of bounds
    current_index %= ARRAYSIZE(new_monsters) - 1;

    FbClear();
    FbColor(BLACK);
    FbImage4bit(new_monsters[current_index].asset, 0);
}

void trade_monsters(void)
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
        state.screen_changed = true;
        state.trading_monsters_enabled = true;
    }
    state.app_state = RENDER_SCREEN;
}
