#include "core.h"
#include "wm.h"

static void keyboard_handle_modifiers(
                struct wl_listener *listener, void *data) {
        /* This event is raised when a modifier key, such as shift or alt, is
         * pressed. We simply communicate this to the client.
         *
         * このイベントは修飾キーが発火する、shiftとかaltとかを、
         * 押したとき。クライアントに押されたことを知らせるだけで他には何もしないよ */
        struct tinywl_keyboard *keyboard =
                wl_container_of(listener, keyboard, modifiers);
        /*
         * A seat can only have one keyboard, but this is a limitation of the
         * Wayland protocol - not wlroots. We assign all connected keyboards to the
         * same seat. You can swap out the underlying wlr_keyboard like this and
         * wlr_seat handles this transparently.
         *
         * シート（ユーザー１人分の操作セット）は一つのキーボードしか持てないよ,でもこの制限は
         * wlプロトコルの制約であってwlrootで変えることはできないよ。このコードは一つのシートに
         * すべてのキーボードを接続してるよ。実際にキーボードを差し替えるときは
         * シートが自動で判定してくれるよ。
         * */
        wlr_seat_set_keyboard(keyboard->server->seat, keyboard->wlr_keyboard);
        /* Send modifiers to the client. */
        /* 修飾キーをクライアントに伝えてるよ*/
        wlr_seat_keyboard_notify_modifiers(keyboard->server->seat,
                &keyboard->wlr_keyboard->modifiers);
}

//キーバインドのイベント
static bool handle_keybinding(struct tinywl_server *server, xkb_keysym_t sym) {
        /*
         * Here we handle compositor keybindings. This is when the compositor is
         * processing keys, rather than passing them on to the client for its own
         * processing.
         *
         * This function assumes Alt is held down.
         *
         * このプログラムはコンポジタのキーバインド設定。コンポジタに設定されたキーバインドは
         * クライアントアプリよりも先に優先されて処理されるよ。
         *
         * このキーはAltが押されている状態を前提としてるよ。
         */
        switch (sym) {
        case XKB_KEY_Escape:
                wl_display_terminate(server->wl_display);
                break;
        case XKB_KEY_F1:
                /* Cycle to the next toplevel */
                /* 直前のウィンドウへ切り替え*/
                if (wl_list_length(&server->toplevels) < 2) {
                        break;
                }
                struct tinywl_toplevel *next_toplevel =
                        wl_container_of(server->toplevels.prev, next_toplevel, link);
                focus_toplevel(next_toplevel);//WM関数なので後でリファクタ
                break;
        case XKB_KEY_XF86Switch_VT_1 ... XKB_KEY_XF86Switch_VT_12: {
                if (server->session) {
                        unsigned vt = sym - XKB_KEY_XF86Switch_VT_1 + 1;
                        wlr_session_change_vt(server->session, vt);
                }
                break;
        }
        default:
                return false;
        }
        return true;
}

//キーボードショートカットのイベント
static void keyboard_handle_key(
                struct wl_listener *listener, void *data) {
        /* This event is raised when a key is pressed or released. */
        /* このイベントはキーを押した・離した時に発火するよ */
        struct tinywl_keyboard *keyboard =
                wl_container_of(listener, keyboard, key);
        struct tinywl_server *server = keyboard->server;
        struct wlr_keyboard_key_event *event = data;
        struct wlr_seat *seat = server->seat;

        /* Translate libinput keycode -> xkbcommon */
        /* libinputのキーコードをxkbに変換 */
        uint32_t keycode = event->keycode + 8;
        /* Get a list of keysyms based on the keymap for this keyboard */
        /* キーボードの配列情報からキーシンボル一覧を取得 */
        const xkb_keysym_t *syms;
        int nsyms = xkb_state_key_get_syms(
                        keyboard->wlr_keyboard->xkb_state, keycode, &syms);

        bool handled = false;
        uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->wlr_keyboard);
        if ((modifiers & WLR_MODIFIER_ALT) &&
                        event->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
                /* If alt is held down and this button was _pressed_, we attempt to
                 * process it as a compositor keybinding.
                 *
                 * altが押されている間、このコードはコンポジタのキーバインドを試すよ*/
                for (int i = 0; i < nsyms; i++) {
                        handled = handle_keybinding(server, syms[i]);
                }
        }

        if (!handled) {
                /* Otherwise, we pass it along to the client.
                 * その他はクライアントに送るよ*/
                wlr_seat_set_keyboard(seat, keyboard->wlr_keyboard);
                wlr_seat_keyboard_notify_key(seat, event->time_msec,
                        event->keycode, event->state);
        }
}

static void keyboard_handle_destroy(struct wl_listener *listener, void *data) {
        /* This event is raised by the keyboard base wlr_input_device to signal
         * the destruction of the wlr_keyboard. It will no longer receive events
         * and should be destroyed.
         *
         * このコードはwlキーボードのインプットメソッドが壊れた時に発火する。
         * キーイベントをこれ以上受け取らないで棄却します。*/
        struct tinywl_keyboard *keyboard =
                wl_container_of(listener, keyboard, destroy);
        wl_list_remove(&keyboard->modifiers.link);
        wl_list_remove(&keyboard->key.link);
        wl_list_remove(&keyboard->destroy.link);
        wl_list_remove(&keyboard->link);
        free(keyboard);
}

static void server_new_keyboard(struct tinywl_server *server,
                struct wlr_input_device *device) {
        struct wlr_keyboard *wlr_keyboard = wlr_keyboard_from_input_device(device);

        struct tinywl_keyboard *keyboard = calloc(1, sizeof(*keyboard));
        keyboard->server = server;
        keyboard->wlr_keyboard = wlr_keyboard;

        /* We need to prepare an XKB keymap and assign it to the keyboard. This
         * assumes the defaults (e.g. layout = "us").
         *
         * このコードはXKBのキーマップを要求し、キーボードに接続します。
         * それをデフォルトとして動作させます（例：layout = "us"）*/
        struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
        struct xkb_keymap *keymap = xkb_keymap_new_from_names(context, NULL,
                XKB_KEYMAP_COMPILE_NO_FLAGS);

        wlr_keyboard_set_keymap(wlr_keyboard, keymap);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        wlr_keyboard_set_repeat_info(wlr_keyboard, 25, 600);

        /* Here we set up listeners for keyboard events. */
        keyboard->modifiers.notify = keyboard_handle_modifiers;
        wl_signal_add(&wlr_keyboard->events.modifiers, &keyboard->modifiers);
        keyboard->key.notify = keyboard_handle_key;
        wl_signal_add(&wlr_keyboard->events.key, &keyboard->key);
        keyboard->destroy.notify = keyboard_handle_destroy;
        wl_signal_add(&device->events.destroy, &keyboard->destroy);
        wlr_seat_set_keyboard(server->seat, keyboard->wlr_keyboard);
        wl_list_insert(&server->keyboards, &keyboard->link);
}