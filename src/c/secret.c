#include <pebble.h>

static unsigned char *encodedData;
static unsigned char *lastEncodedData;
static unsigned encodedDataLength;

static Window *welcomeWin;
static TextLayer *welcomeText;
static ActionBarLayer *welcomeBar;
static GBitmap *hiBM, *lowBM, *okBM;
static const char welcomeLabel[] = "Please introduce the key.";

static unsigned char passKey[20];
static int passKeyLen;

static Window *secretsListWin;
static SimpleMenuLayer *secretsList;
static const char **secretItems;
static SimpleMenuSection mainSection;
static SimpleMenuItem *secretMenuItems;

static Window *deleteDialog;
static ActionBarLayer *dialogBar;
static TextLayer *dialogText;
static GBitmap *deleteBM;
static int secretToDelete;

static void readEncripted(int const itemNr, int const offset) {
    int mySize = persist_exists(itemNr) ? persist_get_size(itemNr) : 0;
    if (mySize == E_DOES_NOT_EXIST) mySize = 0;
    if (itemNr >= 4) {
       if (offset) {
           encodedData = (unsigned char *) malloc(encodedDataLength = offset + 4);
           encodedData[offset] = '\0';
           encodedData[offset + 1] = '\0';
           encodedData[offset + 2] = '\0';
           encodedData[offset + 3] = '\0';
       }
    } else readEncripted(itemNr + 1, offset + mySize);
    if (mySize > 0)
        persist_read_data(itemNr, encodedData + offset, mySize);
}

static void encodeData(unsigned int const itemNr, unsigned int const offset) {
    int const len1 = strlen(secretMenuItems[itemNr].title) + 1,
              len2 = strlen(secretItems[itemNr]) + 1;
    unsigned int const noffset = offset + len1 + len2;
    if (itemNr == 0) {
        if (noffset > encodedDataLength) {
            lastEncodedData = encodedData;
            encodedData = (unsigned char *) malloc(noffset);
        }
        encodedDataLength = noffset;
    } else encodeData(itemNr - 1, noffset);
    if (encodedData + encodedDataLength - noffset != (unsigned char *) secretMenuItems[itemNr].title) {
        memcpy(encodedData + encodedDataLength - noffset, secretMenuItems[itemNr].title, len1);
        memcpy(encodedData + encodedDataLength - noffset + len1, secretItems[itemNr], len2);
    }
}

static void writeEncripted() {
    int needItems;
    if (mainSection.num_items) {
        encodeData(mainSection.num_items - 1, 0);
        if (lastEncodedData) {
            free(lastEncodedData);
            lastEncodedData = 0;
        }
    }
    needItems = (encodedDataLength + PERSIST_DATA_MAX_LENGTH - 1) / PERSIST_DATA_MAX_LENGTH;
    for (int i = needItems; i < 256; ++i) persist_delete(i);
    for (int i = 0; i < needItems ; ++i) {
        unsigned char buffer[PERSIST_DATA_MAX_LENGTH];
        int const bufferlen = persist_read_data(i, buffer, PERSIST_DATA_MAX_LENGTH);
        int const inData = i + 1 < needItems ? PERSIST_DATA_MAX_LENGTH : encodedDataLength % PERSIST_DATA_MAX_LENGTH;
        if (bufferlen != inData && memcmp(buffer, encodedData + i * PERSIST_DATA_MAX_LENGTH, inData))
            persist_write_data(i, encodedData + i * PERSIST_DATA_MAX_LENGTH, inData);
    }
}

static void finishKey(ClickRecognizerRef recognizer, void *context) {
    // decode encoded.
    window_stack_push(secretsListWin, true);
    window_stack_remove(welcomeWin, false);
}

static void addHi(ClickRecognizerRef recognizer, void *context) {
    if (passKeyLen < 160) {
        passKey[passKeyLen / 8] |= 1 << (7 - passKeyLen % 8);
        ++passKeyLen;
    } else finishKey(recognizer, context);
}
static void addLow(ClickRecognizerRef recognizer, void *context) {
    if (passKeyLen < 160) ++passKeyLen;
    else finishKey(recognizer, context);
}

static void setKey(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP, addHi);
    window_single_click_subscribe(BUTTON_ID_SELECT, finishKey);
    window_single_click_subscribe(BUTTON_ID_DOWN, addLow);
}

static void startWelcome(Window *win) {
    Layer *root = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(root);
    GSize needSize;
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    hiBM = gbitmap_create_with_resource(RESOURCE_ID_UP);
    lowBM = gbitmap_create_with_resource(RESOURCE_ID_DOWN);
    okBM = gbitmap_create_with_resource(RESOURCE_ID_CHECK);
    welcomeBar = action_bar_layer_create();
    action_bar_layer_add_to_window(welcomeBar, win);
    action_bar_layer_set_icon_animated(welcomeBar, BUTTON_ID_UP, hiBM, true);
    action_bar_layer_set_icon_animated(welcomeBar, BUTTON_ID_SELECT, okBM, true);
    action_bar_layer_set_icon_animated(welcomeBar, BUTTON_ID_DOWN, lowBM, true);
    action_bar_layer_set_click_config_provider(welcomeBar, setKey);
    bounds.size.w -= ACTION_BAR_WIDTH;
    needSize = graphics_text_layout_get_content_size(welcomeLabel, font, bounds,
            GTextOverflowModeWordWrap, GTextAlignmentLeft);
    bounds.origin.y = (bounds.size.h - needSize.h) / 2;
    bounds.size.h = needSize.h + 10;
    welcomeText = text_layer_create(bounds);
    text_layer_set_overflow_mode(welcomeText, GTextOverflowModeWordWrap);
    text_layer_set_text_alignment(welcomeText, GTextAlignmentCenter);
    text_layer_set_font(welcomeText, font);
    text_layer_set_text(welcomeText, welcomeLabel);
    layer_add_child(root, text_layer_get_layer(welcomeText));

    memset(passKey, 0, 20);
    passKeyLen = 0;
    encodedData = 0;
    readEncripted(0, 0);
    if (!encodedData) {
        encodedData = (unsigned char*)malloc(17*20);
        encodedDataLength = 0;
        for (int i = 0; i < 16; ++i) {
            encodedDataLength += snprintf((char *)encodedData + encodedDataLength, 20, "Titlul:%d", i);
            encodedDataLength++;
            encodedDataLength += snprintf((char *)encodedData + encodedDataLength, 20, "Secretul:%d", i);
            encodedDataLength++;
        }
        encodedData[encodedDataLength++] = '\0';
        encodedData[encodedDataLength++] = '\0';
    }
}

static void finishWelcome(Window *win) {
    text_layer_destroy(welcomeText);
    action_bar_layer_destroy(welcomeBar);
    gbitmap_destroy(hiBM);
    gbitmap_destroy(okBM);
    gbitmap_destroy(lowBM);
}

static void mkDialog(int index, void *context) {
    if (mainSection.num_items > (unsigned) index) {
        secretToDelete = index;
        window_stack_push(deleteDialog, true);
    }
}

static void parseDecoded(const char *decoded, int const itemNr) {
    const char *secr = decoded + 1;
    if (decoded[0] == '\0' && secr[0] == '\0') {
        if (itemNr) {
            secretItems = (const char **) malloc(sizeof(const char *) * (itemNr));
            secretMenuItems = calloc(itemNr, sizeof(SimpleMenuItem));
        }
        mainSection.num_items = itemNr;
        return;
    }
    secr = decoded + strlen(decoded) + 1;
    parseDecoded(secr + strlen(secr) + 1, itemNr + 1);
    secretMenuItems[itemNr].title = decoded;
    secretMenuItems[itemNr].callback = mkDialog;
    secretItems[itemNr] = secr;
}

static void startList(Window *win) {
    Layer *root = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(root);
    unsigned int passDebug;
    char * sectionTitle = (char *)malloc(41);
    memcpy(&passDebug, passKey, sizeof(passDebug));
    parseDecoded((char *)encodedData, 0);
    mainSection.items = secretMenuItems;
    snprintf(sectionTitle, 41, "Keie:%08X", passDebug);
    mainSection.title = sectionTitle;
    secretsList = simple_menu_layer_create(bounds, win, &mainSection, 1, NULL);
    layer_add_child(root, simple_menu_layer_get_layer(secretsList));
}

static void finishList(Window *fer) {
    writeEncripted();
    simple_menu_layer_destroy(secretsList);
    free((char *)mainSection.title);
    free(secretMenuItems);
    free(secretItems);
}

static void doDelete(ClickRecognizerRef recognizer, void *context) {
    if (!mainSection.num_items) return;
    --mainSection.num_items;
    for (unsigned i = secretToDelete; i < mainSection.num_items; ++i) {
        secretItems[i] = secretItems[i + 1];
        secretMenuItems[i] = secretMenuItems[i + 1];
    }
    window_stack_pop(true);
}

static void registerDelete(void *context) {
    window_long_click_subscribe(BUTTON_ID_SELECT, 700, doDelete, NULL);
}

static void startDialog(Window *win) {
    Layer *root = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(root);
    GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    deleteBM = gbitmap_create_with_resource(RESOURCE_ID_DELETE);
    dialogBar = action_bar_layer_create();
    action_bar_layer_add_to_window(dialogBar, win);
    action_bar_layer_set_icon_animated(dialogBar, BUTTON_ID_SELECT, deleteBM, true);
    action_bar_layer_set_click_config_provider(dialogBar, registerDelete);
    bounds.size.w -= ACTION_BAR_WIDTH;
    dialogText = text_layer_create(bounds);
    text_layer_set_overflow_mode(dialogText, GTextOverflowModeWordWrap);
    text_layer_set_font(dialogText, font);
    text_layer_set_text(dialogText, secretItems[secretToDelete]);
    layer_add_child(root, text_layer_get_layer(dialogText));
}

static void finishDialog(Window *win) {
    text_layer_destroy(dialogText);
    action_bar_layer_destroy(dialogBar);
    gbitmap_destroy(deleteBM);
}

int main(void) {
    welcomeWin = window_create();
    window_set_window_handlers(welcomeWin, (WindowHandlers) {
        .load = startWelcome,
        .unload = finishWelcome
    });
    secretsListWin = window_create();
    window_set_window_handlers(secretsListWin, (WindowHandlers) {
        .load = startList,
        .unload = finishList
    });
    deleteDialog = window_create();
    window_set_window_handlers(deleteDialog, (WindowHandlers) {
        .load = startDialog,
        .unload = finishDialog
    });
    window_stack_push(welcomeWin, true);
    app_event_loop();
    free(encodedData);
    window_destroy(secretsListWin);
    window_destroy(welcomeWin);
}