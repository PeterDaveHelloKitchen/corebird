/*  This file is part of corebird, a Gtk+ linux Twitter client.
 *  Copyright (C) 2013 Timm Bäder
 *
 *  corebird is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  corebird is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with corebird.  If not, see <http://www.gnu.org/licenses/>.
 */




[GtkTemplate (ui = "/org/baedert/corebird/ui/user-lists-widget.ui")]
class UserListsWidget : Gtk.Box {
  [GtkChild]
  private Gtk.Label user_list_label;
  [GtkChild]
  private Gtk.ListBox user_list_box;
  [GtkChild]
  private Gtk.Frame user_list_frame;
  [GtkChild]
  private Gtk.Label subscribed_list_label;
  [GtkChild]
  private Gtk.ListBox subscribed_list_box;
  [GtkChild]
  private Gtk.Frame subscribed_list_frame;
  [GtkChild]
  private NewListEntry new_list_entry;

  public unowned MainWindow main_window { get; set; }
  public unowned Account account        { get; set; }
  private bool show_create_entry = true;


  construct {
    user_list_box.set_header_func (header_func);
    user_list_box.set_sort_func (ListListEntry.sort_func);
    subscribed_list_box.set_header_func (header_func);
    subscribed_list_box.set_sort_func (ListListEntry.sort_func);
  }

  public void hide_user_list_entry () {
    new_list_entry.hide ();
    new_list_entry.no_show_all = true;
    show_create_entry = false;
  }

  [GtkCallback]
  private void row_activated (Gtk.ListBoxRow row) {
    if (row is NewListEntry) {
      ((NewListEntry)row).reveal ();
    } else {
      var entry = (ListListEntry) row;
      main_window.switch_page (MainWindow.PAGE_LIST_STATUSES,
                               entry.id,
                               entry.name,
                               entry.user_list,
                               entry.description,
                               entry.creator_screen_name,
                               entry.n_subscribers,
                               entry.n_members,
                               entry.created_at,
                               entry.mode);
    }
  }

  public async void load_lists (int64 user_id) { // {{{
    if (user_id == 0)
      user_id = account.id;


    var call = account.proxy.new_call ();
    call.set_function ("1.1/lists/subscriptions.json");
    call.set_method ("GET");
    call.add_param ("user_id", user_id.to_string ());
    call.invoke_async.begin (null, (obj, res) => {
      uint n_subscribed_list = lists_received_cb (obj, res, subscribed_list_box);
      if (n_subscribed_list == 0) {
        subscribed_list_box.hide ();
        subscribed_list_frame.hide ();
        subscribed_list_label.hide ();
      } else {
        subscribed_list_box.show ();
        subscribed_list_frame.show ();
        subscribed_list_label.show ();
      }
    });


    var user_call = account.proxy.new_call ();
    user_call.set_function ("1.1/lists/ownerships.json");
    user_call.set_method ("GET");
    user_call.add_param ("user_id", user_id.to_string ());
    user_call.invoke_async.begin (null, (obj, res) => {
      uint n_user_list = lists_received_cb (obj, res, user_list_box);
      if (n_user_list == 0 && !show_create_entry) {
        user_list_label.hide ();
        user_list_box.hide ();
        user_list_frame.hide ();
      } else {
        user_list_label.show ();
        user_list_box.show ();
        user_list_frame.show ();
      }
      load_lists.callback ();
    });
    yield;
  } // }}}

  private uint lists_received_cb (GLib.Object? o, GLib.AsyncResult res,
                                  Gtk.ListBox list_box) { // {{{
    var call = (Rest.ProxyCall) o;
    try {
      call.invoke_async.end (res);
    } catch (GLib.Error e) {
      Utils.show_error_object (call.get_payload (), e.message,
                               GLib.Log.LINE, GLib.Log.FILE);
      return 0;
    }
    var parser = new Json.Parser ();
    try {
      parser.load_from_data (call.get_payload ());
    } catch (GLib.Error e) {
      warning (e.message);
      return 0;
    }

    var arr = parser.get_root ().get_object ().get_array_member ("lists");
    arr.foreach_element ((array, index, node) => {
      var obj = node.get_object ();
      var entry = new ListListEntry.from_json_data (obj, account);
      list_box.add (entry);
    });
    return arr.get_length ();
  } // }}}


  public void remove_list (int64 list_id) {
    user_list_box.foreach ((w) => {
      if (!(w is ListListEntry))
        return;

      if (((ListListEntry)w).id == list_id) {
        user_list_box.remove (w);
      }
    });

    subscribed_list_box.foreach ((w) => {
      if (!(w is ListListEntry))
        return;

      if (((ListListEntry)w).id == list_id) {
        subscribed_list_box.remove (w);
      }
    });

    if (subscribed_list_box.get_children ().length () == 0) {
      subscribed_list_label.hide ();
      subscribed_list_frame.hide ();
    }
  }

  public void add_list (ListListEntry entry) {
    if (entry.user_list) {
      // Avoid duplicates
      var user_lists = user_list_box.get_children ();
      foreach (Gtk.Widget w in user_lists) {
        if (!(w is ListListEntry))
          continue;

        if (((ListListEntry)w).id == entry.id)
          return;
      }
      user_list_box.add (entry);
    } else {
      // Avoid duplicates
      var subscribed_lists = subscribed_list_box.get_children ();
      foreach (Gtk.Widget w in subscribed_lists) {
        if (!(w is ListListEntry))
          continue;

        if (((ListListEntry)w).id == entry.id)
          return;
      }
      subscribed_list_box.add (entry);
      subscribed_list_frame.show ();
      subscribed_list_box.show ();
      subscribed_list_label.show ();
    }
  }

  public void update_list (int64 list_id, string name, string description, string mode) {
    user_list_box.foreach ((w) => {
      if (!(w is ListListEntry))
        return;

      var lle = (ListListEntry) w;
      if (lle.id == list_id) {
        lle.name = name;
        lle.description = description;
        lle.mode = mode;
        lle.queue_draw ();
      }
    });
  }

  public void update_member_count (int64 list_id, int increase) {
    var lists = user_list_box.get_children ();
    foreach (var list in lists) {
      if (!(list is ListListEntry))
        continue;

      var lle = (ListListEntry) list;
      if (lle.id == list_id) {
        lle.n_members += increase;
        break;
      }
    }
  }

  public TwitterList[] get_user_lists () { // {{{
    GLib.List<weak Gtk.Widget> children = user_list_box.get_children ();
    TwitterList[] lists = new TwitterList[children.length () - 1];
    int i = 0;
    foreach (Gtk.Widget w in children) {
      if (!(w is ListListEntry))
        continue;

      var lle = (ListListEntry) w;
      lists[i].id = lle.id;
      lists[i].name = lle.name;
      lists[i].description = lle.description;
      lists[i].mode = lle.mode;
      i ++;
    }
    return lists;
  } // }}}

  public void clear_lists () {
    user_list_box.foreach ((w) => { user_list_box.remove (w);});
    subscribed_list_box.foreach ((w) => {subscribed_list_box.remove (w);});
  }

  private void header_func (Gtk.ListBoxRow row, Gtk.ListBoxRow? row_before) { //{{{
    if (row_before == null)
      return;

    Gtk.Widget header = row.get_header ();
    if (header == null) {
      header = new Gtk.Separator (Gtk.Orientation.HORIZONTAL);
      header.show ();
      row.set_header (header);
    }
  } //}}}

  [GtkCallback]
  private void new_list_create_activated_cb (string list_name) { // {{{
    if (list_name.strip ().length <= 0)
      return;

    new_list_entry.sensitive = false;
    var call = account.proxy.new_call ();
    call.set_function ("1.1/lists/create.json");
    call.set_method ("POST");
    call.add_param ("name", list_name);
    call.invoke_async.begin (null, (o, res) => {
      try {
        call.invoke_async.end (res);
      } catch (GLib.Error e) {
        Utils.show_error_object (call.get_payload (), e.message,
                                 GLib.Log.LINE, GLib.Log.FILE);
        new_list_entry.sensitive = true;
        return;
      }
      var parser = new Json.Parser ();
      try {
        parser.load_from_data (call.get_payload ());
      } catch (GLib.Error e) {
        critical (e.message);
        return;
      }
      var root = parser.get_root ().get_object ();
      var entry = new ListListEntry.from_json_data (root, account);
      add_list (entry);

      main_window.switch_page (MainWindow.PAGE_LIST_STATUSES,
                               entry.id,
                               entry.name,
                               true,
                               entry.description,
                               entry.creator_screen_name,
                               entry.n_subscribers,
                               entry.n_members,
                               entry.created_at,
                               entry.mode);
      new_list_entry.sensitive = true;
    });
  } // }}}

  public void unreveal () {
    new_list_entry.unreveal ();
  }
}
