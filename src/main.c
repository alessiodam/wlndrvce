// sorry for the stupid comments but I need them
// this is temporary anyway (or maybe not idk, i hope it is)
// - Alessio

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>

#ifndef GIT_TAG
#define GIT_TAG "v0.0.0"
#endif
#ifndef GIT_SHA
#define GIT_SHA "dev"
#endif

#define COLOR_BG        255
#define COLOR_FG        0
#define COLOR_ACCENT    191
#define COLOR_SELECT    224
#define COLOR_EDIT      229

#define TAB_HEIGHT      20
#define TOP_BAR_HEIGHT  20
#define CONTENT_Y       30
#define ROW_HEIGHT      16

#define COL_TABS_X      0
#define COL_TABS_W      80
#define COL_OPTS_X      85
#define COL_OPTS_W      130
#define COL_VALS_X      195
#define COL_VALS_W      105

#define CONFIG_APPVAR_NAME "WLANCFG"
#define CONFIG_MAGIC_BITES 0x574C414E // get it?

typedef enum {
    TAB_NETWORKS = 0,
    TAB_MISC,
    TAB_EXPERT,
    TAB_COUNT
} ui_tab_t;

typedef enum {
    TYPE_BOOL_TOGGLE,
    TYPE_INT_SLIDER,
    TYPE_ACTION,
    TYPE_LABEL,
    TYPE_INFO,
    TYPE_NET_SLOT
} opt_type_t;

typedef enum {
    OPT_NET_SLOT_1 = 0,
    OPT_NET_SLOT_2,
    OPT_NET_SLOT_3,
    OPT_CONN_STATUS,

    OPT_ENABLE_RADIO,
    OPT_POWER_SAVE,
    OPT_AUTO_CONNECT,

    OPT_TX_POWER,
    OPT_CHANNEL,
    OPT_DEBUG_LOG,
    OPT_MAC_ADDR,

    OPT_COUNT_TOTAL
} config_id_t;

struct config_option;
typedef bool (*action_handler_fn)(struct config_option *opt);

struct config_option {
    const char *name;
    ui_tab_t tab;
    config_id_t id;
    opt_type_t type;
    int32_t value;
    int32_t min_val;
    int32_t max_val;
    action_handler_fn handler;
};

typedef enum {
    FOCUS_TABS = 0,
    FOCUS_OPTIONS
} focus_mode_t;

// I know there's no password
// TODO: expand this to hold real network info
typedef struct {
    bool used;
    char ssid[32];
} net_slot_t;

typedef struct {
    uint32_t magic;
    bool radio_enabled;
    bool power_save;
    bool auto_connect;
    bool debug_log;
    int tx_power;
    int channel;
    uint8_t mac[6];

    net_slot_t slots[3];
    int8_t connected_slot;
} wlan_config_t;

static wlan_config_t g_cfg;
static bool should_we_yes_we_redraw = true;


static void set_defaults(void) {
    memset(&g_cfg, 0, sizeof(wlan_config_t));
    g_cfg.magic = CONFIG_MAGIC_BITES;
    g_cfg.radio_enabled = true;
    g_cfg.power_save = true;
    g_cfg.auto_connect = true;
    g_cfg.tx_power = 100;
    g_cfg.channel = 6;
    g_cfg.connected_slot = -1;
    g_cfg.mac[0]=0x67; g_cfg.mac[1]=0x67; g_cfg.mac[2]=0x67;
    g_cfg.mac[3]=0x67; g_cfg.mac[4]=0x67; g_cfg.mac[5]=0x67;
}

static void load_config(void) {
    uint8_t handle = ti_Open(CONFIG_APPVAR_NAME, "r");
    if (!handle) {
        set_defaults();
        return;
    }
    // fun fact: writing code like this does NOT optimize it.
    if (ti_Read(&g_cfg, sizeof(wlan_config_t), 1, handle) != 1 || g_cfg.magic != CONFIG_MAGIC_BITES) set_defaults();
    ti_Close(handle);
}

static void save_config(void) {
    uint8_t handle = ti_Open(CONFIG_APPVAR_NAME, "w");
    if (handle) {
        ti_Write(&g_cfg, sizeof(wlan_config_t), 1, handle);
        ti_Close(handle);
    }
}

// TODO: actually do a real network scan and connect (or we could make the user crazy by lying)
static bool action_manage_slot(struct config_option *opt) {
    int slot_idx = opt->id - OPT_NET_SLOT_1;
    net_slot_t *slot = &g_cfg.slots[slot_idx];
    if (!slot->used) {
        gfx_SetColor(COLOR_BG);
        gfx_FillRectangle(60, 100, 200, 40);
        gfx_SetColor(COLOR_FG);
        gfx_Rectangle(60, 100, 200, 40);
        gfx_PrintStringXY("Really scanning atm", 80, 115);
        gfx_SwapDraw();
        delay(500);
        snprintf(slot->ssid, 32, "LSJ-%d", rand() % 99);
        slot->used = true;
        g_cfg.connected_slot = slot_idx;
    } else {
        g_cfg.connected_slot = slot_idx;
    }
    return true;
}

// hmmm very clean formatting. I like it.
// I have an exam tomorrow now that I think about it :p
static struct config_option options[] = {
    {"Slot 1",        TAB_NETWORKS, OPT_NET_SLOT_1,   TYPE_NET_SLOT,   0, 0, 0, action_manage_slot},
    {"Slot 2",        TAB_NETWORKS, OPT_NET_SLOT_2,   TYPE_NET_SLOT,   0, 0, 0, action_manage_slot},
    {"Slot 3",        TAB_NETWORKS, OPT_NET_SLOT_3,   TYPE_NET_SLOT,   0, 0, 0, action_manage_slot},
    {"Status:",       TAB_NETWORKS, OPT_CONN_STATUS,  TYPE_INFO,       0, 0, 0, NULL},
    {"Enable Radio",  TAB_MISC,     OPT_ENABLE_RADIO, TYPE_BOOL_TOGGLE, 1, 0, 1, NULL},
    {"Power Save",    TAB_MISC,     OPT_POWER_SAVE,   TYPE_BOOL_TOGGLE, 1, 0, 1, NULL},
    {"Auto Connect",  TAB_MISC,     OPT_AUTO_CONNECT, TYPE_BOOL_TOGGLE, 1, 0, 1, NULL},
    {"Tx Power (%)",  TAB_EXPERT,   OPT_TX_POWER,     TYPE_INT_SLIDER, 100, 10, 100, NULL},
    {"Channel",       TAB_EXPERT,   OPT_CHANNEL,      TYPE_INT_SLIDER, 6, 1, 14, NULL},
    {"Debug Logging", TAB_EXPERT,   OPT_DEBUG_LOG,    TYPE_BOOL_TOGGLE, 0, 0, 1, NULL},
    {"MAC Address",   TAB_EXPERT,   OPT_MAC_ADDR,     TYPE_INFO,       0, 0, 0, NULL},
};

#define OPTION_COUNT (sizeof(options) / sizeof(options[0]))

static void sync_opts_from_config(void) {
    for(size_t i=0; i<OPTION_COUNT; i++) {
        switch(options[i].id) {
            case OPT_ENABLE_RADIO: options[i].value = g_cfg.radio_enabled; break;
            case OPT_POWER_SAVE:   options[i].value = g_cfg.power_save;    break;
            case OPT_AUTO_CONNECT: options[i].value = g_cfg.auto_connect;  break;
            case OPT_TX_POWER:     options[i].value = g_cfg.tx_power;      break;
            case OPT_CHANNEL:      options[i].value = g_cfg.channel;       break;
            case OPT_DEBUG_LOG:    options[i].value = g_cfg.debug_log;     break;
            default: break;
        }
    }
}

static void sync_config_from_options(void) {
    for(size_t i=0; i<OPTION_COUNT; i++) {
        switch(options[i].id) {
            case OPT_ENABLE_RADIO: g_cfg.radio_enabled = options[i].value; break;
            case OPT_POWER_SAVE:   g_cfg.power_save = options[i].value;    break;
            case OPT_AUTO_CONNECT: g_cfg.auto_connect = options[i].value;  break;
            case OPT_TX_POWER:     g_cfg.tx_power = options[i].value;      break;
            case OPT_CHANNEL:      g_cfg.channel = options[i].value;       break;
            case OPT_DEBUG_LOG:    g_cfg.debug_log = options[i].value;     break;
            default: break;
        }
    }
}

static int get_first_opt_idx(ui_tab_t tab) {
    for(int i=0; i<(int)OPTION_COUNT; i++) if(options[i].tab == tab) return i;
    return -1;
}

static int get_next_opt_idx(ui_tab_t tab, int current, int dir) {
    int next = current;
    while(true) {
        next += dir;
        if(next < 0 || next >= (int)OPTION_COUNT) return current;
        if(options[next].tab == tab) return next;
    }
}

void drawTopBar() {
    char topbar[128];
    snprintf(topbar, sizeof(topbar), "WLAN Config %s-%s", GIT_TAG, GIT_SHA);
    gfx_SetColor(COLOR_ACCENT);
    gfx_FillRectangle(0, 0, LCD_WIDTH, TOP_BAR_HEIGHT);
    gfx_SetTextFGColor(COLOR_FG);
    gfx_SetTextXY(5, 6);
    gfx_PrintString(topbar);
}

void drawTabs(ui_tab_t active_tab) {
    const char *names[] = {"Networks", "Misc", "Smart Guy"}; // Smart Guy = Expert because me when me when when me when me
    int y = CONTENT_Y;

    for(int i=0; i<TAB_COUNT; i++) {
        if(i == active_tab) gfx_SetColor(COLOR_ACCENT); else gfx_SetColor(COLOR_BG);
        gfx_FillRectangle(COL_TABS_X, y, COL_TABS_W, TAB_HEIGHT);
        gfx_SetTextFGColor(COLOR_FG);
        gfx_SetTextXY(COL_TABS_X + 5, y + 4);
        gfx_PrintString(names[i]);
        y += TAB_HEIGHT;
    }
    gfx_SetColor(COLOR_FG);
    gfx_VertLine(COL_TABS_X + COL_TABS_W, CONTENT_Y, 200);
    gfx_SetTextFGColor(COLOR_FG);
}

void drawOptionRow(int index, bool is_selected, bool is_editing) {
    struct config_option *opt = &options[index];
    int relative_idx = 0;

    for(int i=0; i<index; i++) if(options[i].tab == opt->tab) relative_idx++;
    int y = CONTENT_Y + relative_idx*ROW_HEIGHT;

    gfx_SetColor(COLOR_BG);
    gfx_FillRectangle(COL_OPTS_X, y, COL_OPTS_W + COL_VALS_W + 10, ROW_HEIGHT);
    if(is_selected) {
        gfx_SetColor(COLOR_ACCENT);
        gfx_FillRectangle(COL_OPTS_X - 4, y + 4, 4, 8);
    }

    gfx_SetTextXY(COL_OPTS_X + 5, y + 4);
    gfx_PrintString(opt->name);

    char val_buf[32] = {0};
    switch(opt->type) {
        case TYPE_BOOL_TOGGLE:
            snprintf(val_buf, sizeof(val_buf), "[%s]", opt->value ? "YES!!" : "NO :(");
            break;
        case TYPE_INT_SLIDER:
            snprintf(val_buf, sizeof(val_buf), "< %d >", (int)opt->value);
            break;
        case TYPE_NET_SLOT: {
            int idx = opt->id - OPT_NET_SLOT_1;
            if (!g_cfg.slots[idx].used) {
                snprintf(val_buf, sizeof(val_buf), "<Empty>");
            } else {
                if (g_cfg.connected_slot == idx) {
                   snprintf(val_buf, sizeof(val_buf), "* %s", g_cfg.slots[idx].ssid);
                } else {
                   snprintf(val_buf, sizeof(val_buf), "%s", g_cfg.slots[idx].ssid);
                }
            }
            break;
        }
        case TYPE_INFO:
            if(opt->id == OPT_CONN_STATUS) {
                if (g_cfg.connected_slot != -1) {
                    snprintf(val_buf, sizeof(val_buf), "Connected");
                } else {
                    snprintf(val_buf, sizeof(val_buf), "Disconnected");
                }
            } else if (opt->id == OPT_MAC_ADDR) {
                snprintf(
                    val_buf,
                    sizeof(val_buf),
                    "%02X:%02X:%02X:%02X:%02X:%02X",
                    g_cfg.mac[0],
                    g_cfg.mac[1],
                    g_cfg.mac[2],
                    g_cfg.mac[3],
                    g_cfg.mac[4],
                    g_cfg.mac[5]
                );
            }
            break;
        default: break;
    }

    if(strlen(val_buf) > 0) {
        if (is_editing && opt->type == TYPE_INT_SLIDER) {
            gfx_SetColor(COLOR_EDIT);
            gfx_FillRectangle(COL_VALS_X - 2, y, COL_VALS_W, ROW_HEIGHT);
        }
        int max_width = 320 - COL_VALS_X - 5;
        int str_width = gfx_GetStringWidth(val_buf);
        if (str_width > max_width) {
             int len = strlen(val_buf);
             while (len > 3 && gfx_GetStringWidth(val_buf) > max_width-10) val_buf[--len] = 0;
             if (len < 31) strcat(val_buf, "..");
        }
        gfx_SetTextXY(COL_VALS_X, y + 4);
        gfx_PrintString(val_buf);
    }
}

void drawCurrentTabContent(ui_tab_t tab, int selected_idx, bool editing) {
    int current_idx = get_first_opt_idx(tab);
    while(current_idx != -1 && options[current_idx].tab == tab) {
        drawOptionRow(current_idx, current_idx == selected_idx, (current_idx == selected_idx) && editing);
        int prev_idx = current_idx;
        current_idx = get_next_opt_idx(tab, current_idx, 1);
        if(current_idx == prev_idx) break;
        if(current_idx == get_first_opt_idx(tab)) break;
    }
}

void drawBottomBar(void) {
    int y = 220;
    gfx_SetColor(COLOR_BG);
    gfx_FillRectangle(0, y, LCD_WIDTH, 20);
    const char *text = "[Enter] Engage  [Del] Delete  [Clear] Back";
    gfx_SetTextXY((LCD_WIDTH - gfx_GetStringWidth(text)) / 2, y+6);
    gfx_PrintString(text);
}

int main(void) {
    load_config();
    gfx_Begin();
    gfx_SetDrawBuffer();

    ui_tab_t current_tab = TAB_NETWORKS;
    int selection[TAB_COUNT];
    for(int i=0; i<TAB_COUNT; i++) selection[i] = get_first_opt_idx((ui_tab_t)i);
    focus_mode_t focus = FOCUS_TABS;
    bool editing_question_mark = false;

    sync_opts_from_config();

    while(1) {
        if(should_we_yes_we_redraw) {
            gfx_SetColor(COLOR_BG);
            gfx_FillScreen(COLOR_BG);
            drawTopBar();
            drawTabs(current_tab);
            if(selection[current_tab] != -1) drawCurrentTabContent(current_tab, focus == FOCUS_OPTIONS ? selection[current_tab] : -1, editing_question_mark);
            drawBottomBar();
            gfx_SwapDraw();
            should_we_yes_we_redraw = false;
        }

        kb_Scan();

        if (editing_question_mark) {
             struct config_option *opt = &options[selection[current_tab]];
             if (kb_Data[6] & kb_Clear) {
                 editing_question_mark = false;
                 should_we_yes_we_redraw = true;
                 delay(150);
             } else if (kb_Data[7] & kb_Left) {
                 if(opt->value > opt->min_val) {
                     opt->value--;
                     sync_config_from_options();
                     should_we_yes_we_redraw = true;
                     delay(50);
                 }
             } else if (kb_Data[7] & kb_Right) {
                 if(opt->value < opt->max_val) {
                     opt->value++;
                     sync_config_from_options();
                     should_we_yes_we_redraw = true;
                     delay(50);
                 }
             }
        }
        else {
            if (kb_Data[6] & kb_Clear && focus == FOCUS_TABS) break;
            if (focus == FOCUS_TABS) {
                if (kb_Data[7] & kb_Down) {
                    int next = current_tab + 1;
                    if(next < TAB_COUNT) {
                        current_tab = (ui_tab_t)next;
                        should_we_yes_we_redraw = true;
                        delay(150);
                    }
                }
                if (kb_Data[7] & kb_Up) {
                    int prev = current_tab - 1;
                    if(prev >= 0) {
                        current_tab = (ui_tab_t)prev;
                        should_we_yes_we_redraw = true;
                        delay(150);
                    }
                }
                if (kb_Data[7] & kb_Right || kb_Data[6] & kb_Enter) {
                    if(selection[current_tab] != -1) {
                        focus = FOCUS_OPTIONS;
                        should_we_yes_we_redraw = true;
                        delay(150);
                    }
                }
            } else if (focus == FOCUS_OPTIONS) {
                if (kb_Data[6] & kb_Clear) {
                    focus = FOCUS_TABS;
                    should_we_yes_we_redraw = true;
                    delay(150);
                } else if (kb_Data[7] & kb_Down) {
                    int next = get_next_opt_idx(current_tab, selection[current_tab], 1);
                    if(next != selection[current_tab]) {
                        selection[current_tab] = next;
                        should_we_yes_we_redraw = true;
                        delay(150);
                    }
                } else if (kb_Data[7] & kb_Up) {
                    int prev = get_next_opt_idx(current_tab, selection[current_tab], -1);
                    if(prev != selection[current_tab]) {
                        selection[current_tab] = prev;
                        should_we_yes_we_redraw = true;
                        delay(150);
                    }
                }

                if (kb_Data[6] & kb_Del || kb_Data[1] & kb_Del) {
                     struct config_option *opt = &options[selection[current_tab]];
                     if (opt->type == TYPE_NET_SLOT) {
                         int idx = opt->id - OPT_NET_SLOT_1;
                         if (g_cfg.slots[idx].used) {
                             g_cfg.slots[idx].used = false;
                             memset(g_cfg.slots[idx].ssid, 0, 32);
                             if (g_cfg.connected_slot == idx) g_cfg.connected_slot = -1;
                             should_we_yes_we_redraw = true;
                             delay(200);
                         }
                     }
                }

                if (kb_Data[6] & kb_Enter) {
                    struct config_option *opt = &options[selection[current_tab]];
                    if(opt->type == TYPE_BOOL_TOGGLE) {
                        opt->value = !opt->value;
                        sync_config_from_options();
                        should_we_yes_we_redraw = true;
                        delay(200);
                    } else if((opt->type == TYPE_ACTION || opt->type == TYPE_NET_SLOT) && opt->handler) {
                        if(opt->handler(opt)) {
                            should_we_yes_we_redraw = true;
                        }
                        delay(200);
                    } else if(opt->type == TYPE_INT_SLIDER) {
                        editing_question_mark = true;
                        should_we_yes_we_redraw = true;
                        delay(200);
                    }
                }
            }
        }
    }

    gfx_End();
    save_config();
    return 0;
}
