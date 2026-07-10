#pragma once

#include <thread>
#include <glm.h>

class BitmapFont;
class Sprite;
class SongDatabase;
class TruetypeFont;
class LuaManager;
class GraphicalString;
class DrawCallSink;

namespace otoworm {
    class ChartGroup;
}

/*
    The flow of the wheel is as follows.

    When the mouse hovers over an item, the user is notified. When it clicks, the user is notified too.
    The user decides on these two events whether the index is selected or not.
    Hover is called only once every change. Click is only called when the assigned buttons to KT_Select are pressed.

    When an item is selected once and it's a directory, we move over to it.
    If it's not, we set it as a tentative song pick. If it's selected again, we confirm a song was selected.
*/

typedef std::function<void(std::shared_ptr<otoworm::ChartGroup>, uint8_t)> SongNotification;
typedef std::function<void(int32_t, uint32_t, std::string, std::shared_ptr<otoworm::ChartGroup>)> ItemNotification;
typedef std::function<void(int32_t, std::shared_ptr<otoworm::ChartGroup>, bool, int32_t)> ItemTransformFunction;
typedef std::function<void(int32_t, std::shared_ptr<otoworm::ChartGroup>, bool, int32_t, std::string)> StringTransformFunction;
typedef std::function <float(float)> ListTransformFunction;
typedef std::function<void()> DirectoryChangeNotifyFunction;
typedef std::function<bool(const ListEntry * const)> FuncFilterCriteria;

class SongWheel
{
private:
    friend class ScreenSelectMusic;
    SongWheel();

    int32_t cursor_pos_{}, old_cursor_pos_{};
    int32_t selected_bound_item_{}, selected_unbound_item_{};

    std::mutex* m_load_mutex_;
    std::thread* m_load_thread_;
    std::atomic<bool> m_loading_;

    SongDatabase* song_db_{};

    std::shared_ptr<SongList> list_root_;
    SongList* current_list_;
    SongList filtered_current_list_;

    std::map<int, Sprite*> sprites_;
    std::map<int, GraphicalString*> strings_;

    float time_{};

    // itemFraction = item / total of displayed items
    void display_item(int32_t ListItem, int32_t ItemPosition, float itemFraction, DrawCallSink &sink);
    bool in_wheel_bounds(Vec2 Pos);

    bool is_initialized_;
    bool is_hovering_;

    bool loaded_songs_once_;
    size_t difficulty_index_;

    std::vector<FuncFilterCriteria> active_filters_;

    AABBd item_box_at(float t) const;
public:

    DirectoryChangeNotifyFunction on_directory_change;

    ItemNotification on_item_click;
    ItemNotification on_item_hover;
    ItemNotification on_item_hover_leave;

    SongNotification on_song_confirm;
    SongNotification on_song_tentative_select;

    ListTransformFunction transform_horizontal;
    ListTransformFunction transform_vertical;
    ListTransformFunction transform_width;
    ListTransformFunction transform_height;

    int display_item_count;
    int display_start_index;

    ItemTransformFunction transform_item;
    StringTransformFunction transform_string;

    // Singleton
    static SongWheel& get_instance();

    void clean_items();

    void reapply_filters();

    void go_up();
    void initialize(SongDatabase* database);

    void join_loading_thread();

    bool handle_input(int32_t key, bool is_pressed, bool is_mouse_input);
    bool handle_scroll_input(const double dx, const double dy);
    std::shared_ptr<otoworm::ChartGroup> get_selected_chart_group() const;
    void reload_songs(SongDatabase* database);
    void load_songs_once(SongDatabase* database);

    int add_sprite(Sprite* item);
    int add_text(GraphicalString* str);

    // return: the new difficulty index
    int next_difficulty();
    int prev_difficulty();
    size_t get_difficulty() const;
    void set_difficulty(uint32_t i);

    // Returns the index of the last item the user hovered with the mouse over.
    int get_cursor_index() const;
    void set_cursor_index(int index);

    void confirm_selection();

    // Returns the item index the mouse is currently hovering over.
    int get_list_cursor_index() const;

    // These give and set the global, infinite wheel item.
    // When wanting to use the bound index, read from SelectedBoundItem, not these.
    void  set_selected_item(int32_t item);
    int32_t get_selected_item() const;
    int32_t get_num_items() const;

    bool is_item_directory(int32_t item) const;

    int32_t index_at_point(float x, float y) const;
    uint32_t normalized_index_at_point(float x, float y) const;

    bool is_loading();

    void sort_by(ESortCriteria criteria);

    void reset_filters();
    void select_by(const FuncFilterCriteria &criteria);

    void update(float delta);
    void emit_draw_calls(DrawCallSink &sink);
};
