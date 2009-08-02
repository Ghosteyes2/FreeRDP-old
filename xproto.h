#ifndef __XPROTO_H
#define __XPROTO_H

void xclip_handle_SelectionNotify(XSelectionEvent * event);
void xclip_handle_SelectionRequest(XSelectionRequestEvent * xevent);
void xclip_handle_SelectionClear(void);
void xclip_handle_PropertyNotify(XPropertyEvent * xev);
int ewmh_get_window_state(Window w);
int ewmh_change_state(Window wnd, int state);
int ewmh_move_to_desktop(Window wnd, unsigned int desktop);
int ewmh_get_window_desktop(Window wnd);
void ewmh_set_wm_name(Window wnd, const char *title);
int ewmh_set_window_popup(Window wnd);
int ewmh_set_window_modal(Window wnd);
void ewmh_set_icon(Window wnd, int width, int height, const char *rgba_data);
void ewmh_del_icon(Window wnd, int width, int height);
int ewmh_set_window_above(Window wnd);

#endif // __XPROTO_H
