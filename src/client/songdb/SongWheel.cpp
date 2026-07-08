#include <cstdint>
#include <mutex>
#include <ChartGroup.h>
#include <functional>

#include <utility>
#include <glm.h>
#include <rmath.h>

#include "Logging.h"

#include "../game/PlayscreenParameters.h"
#include "../game/GameState.h"

#include "GameWindow.h"
#include "SongLoader.h"

#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"
#include "GraphicalString.h"

#include "SongList.h"
#include "SongWheel.h"
#include "SongList.h"

#include "SongDatabase.h"
#include "../structure/Configuration.h"
#include "../game/Game.h"
//#include <glm/gtc/matrix_transform.inl>

using namespace rd;

SongWheel::SongWheel()
{
    is_initialized_ = false;
    m_load_mutex_ = nullptr;
    m_load_thread_ = nullptr;
    difficulty_index_ = 0;

	display_start_index = 0;
	display_item_count = 0;

    list_root_ = nullptr;
    current_list_ = nullptr;
    loaded_songs_once_ = false;
    is_hovering_ = false;
}

size_t SongWheel::get_difficulty() const
{
    return difficulty_index_;
}

SongWheel& SongWheel::get_instance()
{
    static auto wheel_instance = new SongWheel();
    return *wheel_instance;
}

void SongWheel::clean_items()
{
    strings_.clear();
    sprites_.clear();
}


void SongWheel::initialize(SongDatabase* database)
{
    if (is_initialized_)
    {
        clean_items();
        return;
    }

    selected_bound_item_ = 0;
    selected_unbound_item_ = 0;
    cursor_pos_ = 0;
    old_cursor_pos_ = 0;
    time_ = 0;

    is_initialized_ = true;
    difficulty_index_ = 0;

    load_songs_once(database);
}

class LoadThread
{
    std::mutex* load_mutex_;
    SongDatabase* db_;
    std::shared_ptr<SongList> list_root_;
    std::atomic<bool>& is_loading_;
public:
    LoadThread(std::mutex* m, SongDatabase* d, std::shared_ptr<SongList> r, std::atomic<bool>& loadingstatus)
        : load_mutex_(m),
        db_(d),
        list_root_(std::move(r)),
        is_loading_(loadingstatus)
    {
        is_loading_ = true;
    }

    void load() const {
        std::map<std::string, std::string> directories;

        Configuration::GetConfigListS("SongDirectories", directories, "Songs");

        SongLoader loader(db_);

        Log::Printf("Started loading songs..\n");
        db_->start_transaction();

        for (auto& directory : directories)
        {
            list_root_->add_named_directory(*load_mutex_, loader, directory.second, directory.first, [] {
                SongWheel::get_instance().reapply_filters();
            });
        }

        db_->end_transaction();
        Log::Printf("Finished reloading songs.\n");
        is_loading_ = false;
    }
};

void SongWheel::join_loading_thread()
{
    if (m_load_thread_)
    {
        m_load_thread_->join();
        delete m_load_thread_;
        m_load_thread_ = nullptr;
    }
}

void SongWheel::reload_songs(SongDatabase* database)
{
    song_db_ = database;
    join_loading_thread();

    list_root_ = std::make_shared<SongList>();
    current_list_ = list_root_.get();

    if (!m_load_mutex_)
        m_load_mutex_ = new std::mutex;

    LoadThread loader(m_load_mutex_, song_db_, list_root_, m_loading_);
    m_load_thread_ = new std::thread(&LoadThread::load, loader);
}

void SongWheel::load_songs_once(SongDatabase* database)
{
    if (!loaded_songs_once_) loaded_songs_once_ = true;
    else return;
    reload_songs(database);
}

int SongWheel::add_sprite(Sprite* Item)
{
    const auto size = sprites_.size() + 1;
    sprites_[size] = Item;
    return size;
}

int SongWheel::add_text(GraphicalString* Str)
{
    const auto size = strings_.size() + 1;
    strings_[size] = Str;
    return size;
}

int SongWheel::get_cursor_index() const
{
    size_t Size;
    Size = get_num_items();

    if (Size)
    {
        int ret = cursor_pos_ % (int)Size;
        while (ret < 0)
            ret += Size;
        return ret;
    }

    return 0;
}

int SongWheel::prev_difficulty()
{
    size_t max_index = 0;
    if (!filtered_current_list_.is_directory(selected_bound_item_))
    {
        difficulty_index_--;
        const auto song = filtered_current_list_.get_song_entry(selected_bound_item_);
		max_index = song->charts.size() - 1;

        difficulty_index_ = std::min(max_index, difficulty_index_);
        on_song_tentative_select(get_selected_chart_group(), difficulty_index_);
    }
    else
        difficulty_index_ = 0;

    return difficulty_index_;
}

int SongWheel::next_difficulty()
{
    if (!filtered_current_list_.is_directory(selected_bound_item_))
    {
        difficulty_index_++;

        const auto song = filtered_current_list_.get_song_entry(selected_bound_item_);
        if (difficulty_index_ >= song->charts.size())
            difficulty_index_ = 0;
        

        on_song_tentative_select(get_selected_chart_group(), difficulty_index_);
    }

    return difficulty_index_;
}

bool SongWheel::in_wheel_bounds(const Vec2 Pos)
{
	return index_at_point(Pos.x, Pos.y) != -1;
}

AABBd SongWheel::item_box_at(const float t) const
{
	const Vec2 Position(transform_horizontal(t), transform_vertical(t));
	const Vec2 Size(transform_width(t), transform_height(t));

	// az: No + operator, lol?
	Vec2 BottomRightCorner = Position;
	BottomRightCorner += Size;

	return AABBd(Position.x, Position.y, BottomRightCorner.x, BottomRightCorner.y);
}

void SongWheel::set_difficulty(const uint32_t i)
{
    if (!filtered_current_list_.is_directory(selected_bound_item_))
    {
        const auto song = get_selected_chart_group();
        const size_t max_index = song->charts.size();
        const size_t old_di = difficulty_index_;

        if (max_index)
            difficulty_index_ = clamp(i, uint32_t(0), uint32_t(max_index - 1));
        else
            difficulty_index_ = 0;

        if (difficulty_index_ != old_di)
            on_song_tentative_select(get_selected_chart_group(), difficulty_index_);
    }
}

bool SongWheel::handle_input(const int32_t key, const bool is_pressed, const bool is_mouse_input)
{
    if (is_pressed)
    {
        switch (BindingsManager::translate_key(key))
        {
        default:
            break;
        case KT_Up:
            cursor_pos_--;
            return true;
        case KT_Down:
            cursor_pos_++;
            return true;
        case KT_Select:
            const Vec2 mpos = window.get_relative_mouse_pos();
            const auto boundIndex = get_cursor_index();
            const auto Idx = get_list_cursor_index();
            if (boundIndex != filtered_current_list_.get_num_entries()) // There's entries!
            {
                if (in_wheel_bounds(mpos) || !is_mouse_input)
                {
                    if (on_item_click)
                        on_item_click(Idx, boundIndex,
							filtered_current_list_.get_entry_title(boundIndex),
							filtered_current_list_.get_song_entry(boundIndex));
                    return true;
                }
            }
        }

        switch (key)
        {
        case 'E':
            go_up();
            return true;
        default: ;
        }
    }

    return false;
}

void SongWheel::go_up()
{
    std::unique_lock lock(*m_load_mutex_);

    if (current_list_->has_parent_directory())
    {
		current_list_->set_in_use(false);
        current_list_ = current_list_->get_parent_directory();
		current_list_->clear_empty();
		reapply_filters();
        on_directory_change();
		on_song_tentative_select(get_selected_chart_group(), 0);
    }
}

bool SongWheel::handle_scroll_input(const double dx, const double dy)
{
    return true;
}

std::shared_ptr<otoworm::ChartGroup> SongWheel::get_selected_chart_group()
{
    return filtered_current_list_.get_song_entry(selected_bound_item_);
}

void SongWheel::update(const float delta)
{
    time_ += delta;

    if (!is_loading() && m_load_thread_)
    {
        m_load_thread_->join();
        delete m_load_thread_;
        m_load_thread_ = nullptr;
    }

    if (!current_list_)
        return;

    const Vec2 mpos = window.get_relative_mouse_pos();
    if (in_wheel_bounds(mpos))
    {
        is_hovering_ = true;
        cursor_pos_ = index_at_point(mpos.x, mpos.y);
    }
    else
    {
        if (is_hovering_)
        {
            is_hovering_ = false;
            if (on_item_hover_leave)
                on_item_hover_leave(get_cursor_index(), get_list_cursor_index(),
                filtered_current_list_.get_entry_title(get_cursor_index()), nullptr);
        }
    }

    // Hey we've got a new cursor position, update.
    if (old_cursor_pos_ != cursor_pos_)
    {
        old_cursor_pos_ = cursor_pos_;
        if (on_item_hover)
        {
            const std::shared_ptr<otoworm::ChartGroup> Notify = get_selected_chart_group();
            on_item_hover(get_cursor_index(), get_list_cursor_index(),
                filtered_current_list_.get_entry_title(get_cursor_index()), Notify);
        }
    }
}

void SongWheel::display_item(const int32_t list_item, const int32_t list_position, const float item_fraction)
{
	AABBd screen_box (0.0, 0.0, ScreenWidth, ScreenHeight);
	const AABBd item_box = item_box_at(item_fraction);
	const Vec2 pos(item_box.X1, item_box.Y1);
	// Vec2 size (item_box.width(), item_box.height()); 

    if (screen_box.intersects(item_box))
    {
        bool is_selected = false;
        std::shared_ptr<otoworm::ChartGroup> song = nullptr;
        std::string Text;

        if (list_item != -1)
        {
            song = filtered_current_list_.get_song_entry(list_item);
            Text = filtered_current_list_.get_entry_title(list_item);
            is_selected = (list_position == selected_unbound_item_);
        }

        for (const auto &[index, sprite] : sprites_)
        {			
            sprite->set_position(item_box.X1, item_box.Y1);

            if (transform_item)
                transform_item(index, song, is_selected, list_position);

            // Render the objects.
            sprite->render();
        }

        for (const auto &[index, str] : strings_)
        {
            str->set_position(pos);

            if (transform_string)
                transform_string(index, song, is_selected, list_position, Text);

            str->render();
        }
    }
}


void SongWheel::render()
{
    std::unique_lock<std::mutex> lock(*m_load_mutex_);
    int cur = 0;
    const int max = filtered_current_list_.get_num_entries();

	// I only really need the top index.
	const int display_end_index = display_start_index + display_item_count + 1;

	const float total_entries = display_end_index - display_start_index;
    for (cur = display_start_index; cur <= display_end_index; cur++)
    {
		// Cur*ItemHeight + shownListY

		const float t = (cur - display_start_index) / total_entries;
        if (max)
        {
            int real_index = cur % max;

            while (real_index < 0) // Loop over..
                real_index += max;

            display_item(real_index, cur, t);
        }
        else
            display_item(-1, cur, t);
    }
}

int32_t SongWheel::get_selected_item() const
{
    return selected_unbound_item_;
}


void SongWheel::set_selected_item(int32_t item)
{
    if (!current_list_)
        return;

    selected_unbound_item_ = item;

    // Get a bound item index
	if (get_num_items())
		item = modulo(item, get_num_items());
	else
		item = 0;
    
    // Set bound item index to this.
    selected_bound_item_ = item;
    on_song_tentative_select(get_selected_chart_group(), difficulty_index_);
}

int32_t SongWheel::index_at_point(const float x, const float y) const {
	for (int cur = display_start_index;
		cur != display_start_index + display_item_count + 1;
		cur++) {
		const float t = float(cur - display_start_index) / float(display_item_count + 1);

		if (item_box_at(t).is_in_box(x, y))
			return cur;
	}

	return -1;
}

uint32_t SongWheel::normalized_index_at_point(const float x, const float y) const {
    const auto idx = modulo(index_at_point(x, y), get_num_items());
    return idx;
}

int32_t SongWheel::get_num_items() const
{
    if (!current_list_)
        return 0;
    else
    {
        return filtered_current_list_.get_num_entries();
    }
}

bool SongWheel::is_item_directory(int32_t item) const
{
    if (filtered_current_list_.get_num_entries())
    {
        while (item < 0) item += get_num_items();
        item %= get_num_items();
        return filtered_current_list_.is_directory(item);
    }

    return false;
}

void SongWheel::set_cursor_index(const int index)
{
    cursor_pos_ = index;
}

void SongWheel::confirm_selection()
{
    if (!filtered_current_list_.is_directory(selected_bound_item_))
    {
		if (difficulty_index_ < get_selected_chart_group()->get_chart_count())
			on_song_confirm(get_selected_chart_group(), difficulty_index_);
    }
    else
    {
        current_list_ = current_list_->get_list_entry(selected_bound_item_).get();
		current_list_->set_in_use(true);

		reapply_filters();
        set_selected_item(selected_unbound_item_); // Update our selected item to new bounderies.
        on_song_tentative_select(get_selected_chart_group(), difficulty_index_);
    }
}

int SongWheel::get_list_cursor_index() const
{
    return cursor_pos_;
}

bool SongWheel::is_loading()
{
    if (m_load_thread_)
    {
        return m_loading_;
    }
    else return false;
}

void SongWheel::sort_by(const ESortCriteria criteria)
{
	std::unique_lock<std::mutex> lock(*m_load_mutex_);
	list_root_->sort_by(criteria);
	reapply_filters();
}

void SongWheel::reapply_filters()
{
	if (!current_list_) return;

	filtered_current_list_.clear();
	for (const auto& entry : current_list_->get_entries()) {
		bool add = true;

		if (std::holds_alternative<std::shared_ptr<otoworm::ChartGroup>>(entry.data)) {
			auto song = std::get<std::shared_ptr<otoworm::ChartGroup>>(entry.data);
			if (!GameState::get_instance().is_song_unlocked(song.get()))
				continue;
		}

		// all filters pass?
		// no filters means next block is skipped, so always adds
		for (const auto& can_add : active_filters_) {
			// this one doesn't
			if (!can_add(&entry)) {
				add = false;
				break;
			}
		}

		// all filters passed!
		if (add)
			filtered_current_list_.add_entry(entry);
	}
}

void SongWheel::reset_filters()
{
	active_filters_.clear();
	reapply_filters();
}

void SongWheel::select_by(const FuncFilterCriteria& criteria)
{
	active_filters_.push_back(criteria);
	reapply_filters();
}
