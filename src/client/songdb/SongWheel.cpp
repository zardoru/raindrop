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

int SongWheel::get_difficulty() const
{
    return difficulty_index_;
}

SongWheel& SongWheel::get_instance()
{
    static auto WheelInstance = new SongWheel();
    return *WheelInstance;
}

void SongWheel::clean_items()
{
    strings_.clear();
    sprites_.clear();
}


void SongWheel::initialize(SongDatabase* Database)
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

    load_songs_once(Database);
}

class LoadThread
{
    std::mutex* mLoadMutex;
    SongDatabase* DB;
    std::shared_ptr<SongList> ListRoot;
    std::atomic<bool>& isLoading;
public:
    LoadThread(std::mutex* m, SongDatabase* d, std::shared_ptr<SongList> r, std::atomic<bool>& loadingstatus)
        : mLoadMutex(m),
        DB(d),
        ListRoot(std::move(r)),
        isLoading(loadingstatus)
    {
        isLoading = true;
    }

    void Load()
    {
        std::map<std::string, std::string> Directories;

        Configuration::GetConfigListS("SongDirectories", Directories, "Songs");

        SongLoader Loader(DB);

        Log::Printf("Started loading songs..\n");
        DB->StartTransaction();

        for (auto & Directorie : Directories)
        {
            ListRoot->AddNamedDirectory(*mLoadMutex, &Loader, Directorie.second, Directorie.first, [] {
                SongWheel::get_instance().reapply_filters();
            });
        }

        DB->EndTransaction();
        Log::Printf("Finished reloading songs.\n");
        isLoading = false;
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

void SongWheel::reload_songs(SongDatabase* Database)
{
    db_ = Database;
    join_loading_thread();

    list_root_ = std::make_shared<SongList>();
    current_list_ = list_root_.get();

    if (!m_load_mutex_)
        m_load_mutex_ = new std::mutex;

    LoadThread L(m_load_mutex_, db_, list_root_, m_loading_);
    m_load_thread_ = new std::thread(&LoadThread::Load, L);
}

void SongWheel::load_songs_once(SongDatabase* Database)
{
    if (!loaded_songs_once_) loaded_songs_once_ = true;
    else return;
    reload_songs(Database);
}

int SongWheel::add_sprite(Sprite* Item)
{
    auto size = sprites_.size() + 1;
    sprites_[size] = Item;
    return size;
}

int SongWheel::add_text(GraphicalString* Str)
{
    auto size = strings_.size() + 1;
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
    if (!filtered_current_list_.IsDirectory(selected_bound_item_))
    {
        difficulty_index_--;
        auto song = filtered_current_list_.GetSongEntry(selected_bound_item_);
		max_index = song->charts.size() - 1;

        difficulty_index_ = std::min(max_index, difficulty_index_);
        on_song_tentative_select(GetSelectedChartGroup(), difficulty_index_);
    }
    else
        difficulty_index_ = 0;

    return difficulty_index_;
}

int SongWheel::next_difficulty()
{
    if (!filtered_current_list_.IsDirectory(selected_bound_item_))
    {
        difficulty_index_++;
        
        auto song = filtered_current_list_.GetSongEntry(selected_bound_item_);
        if (difficulty_index_ >= song->charts.size())
            difficulty_index_ = 0;
        

        on_song_tentative_select(GetSelectedChartGroup(), difficulty_index_);
    }

    return difficulty_index_;
}

bool SongWheel::in_wheel_bounds(Vec2 Pos)
{
	return index_at_point(Pos.x, Pos.y) != -1;
}

AABBd SongWheel::item_box_at(float t)
{
	Vec2 Position(transform_horizontal(t), transform_vertical(t));
	Vec2 Size(transform_width(t), transform_height(t));

	// az: No + operator, lol?
	Vec2 BottomRightCorner = Position;
	BottomRightCorner += Size;

	return AABBd(Position.x, Position.y, BottomRightCorner.x, BottomRightCorner.y);
}

void SongWheel::set_difficulty(uint32_t i)
{
    if (!filtered_current_list_.IsDirectory(selected_bound_item_))
    {
        auto song = GetSelectedChartGroup();
        size_t maxIndex = song->charts.size();
        size_t oldDI = difficulty_index_;

        if (maxIndex)
            difficulty_index_ = clamp(i, uint32_t(0), uint32_t(maxIndex - 1));
        else
            difficulty_index_ = 0;

        if (difficulty_index_ != oldDI)
            on_song_tentative_select(GetSelectedChartGroup(), difficulty_index_);
    }
}

bool SongWheel::handle_input(int32_t key, bool isPressed, bool isMouseInput)
{
    if (isPressed)
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
            Vec2 mpos = window.get_relative_mouse_pos();
            auto boundIndex = get_cursor_index();
            auto Idx = get_list_cursor_index();
            if (boundIndex != filtered_current_list_.GetNumEntries()) // There's entries!
            {
                if (in_wheel_bounds(mpos) || !isMouseInput)
                {
                    if (on_item_click)
                        on_item_click(Idx, boundIndex,
							filtered_current_list_.GetEntryTitle(boundIndex),
							filtered_current_list_.GetSongEntry(boundIndex));
                    return true;
                }
            }
        }

        switch (key)
        {
        case 'E':
            go_up();
            return true;
        }
    }

    return false;
}

void SongWheel::go_up()
{
    std::unique_lock<std::mutex> lock(*m_load_mutex_);

    if (current_list_->HasParentDirectory())
    {
		current_list_->SetInUse(false);
        current_list_ = current_list_->GetParentDirectory();
		current_list_->ClearEmpty();
		reapply_filters();
        on_directory_change();
		on_song_tentative_select(GetSelectedChartGroup(), 0);
    }
}

bool SongWheel::handle_scroll_input(const double dx, const double dy)
{
    return true;
}

std::shared_ptr<otoworm::ChartGroup> SongWheel::GetSelectedChartGroup()
{
    return filtered_current_list_.GetSongEntry(selected_bound_item_);
}

void SongWheel::update(float Delta)
{
    uint32_t Size = get_num_items();

    time_ += Delta;

    if (!is_loading() && m_load_thread_)
    {
        m_load_thread_->join();
        delete m_load_thread_;
        m_load_thread_ = nullptr;
    }

    if (!current_list_)
        return;

    Vec2 mpos = window.get_relative_mouse_pos();
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
                filtered_current_list_.GetEntryTitle(get_cursor_index()), nullptr);
        }
    }

    // Hey we've got a new cursor position, update.
    if (old_cursor_pos_ != cursor_pos_)
    {
        old_cursor_pos_ = cursor_pos_;
        if (on_item_hover)
        {
            std::shared_ptr<otoworm::ChartGroup> Notify = GetSelectedChartGroup();
            on_item_hover(get_cursor_index(), get_list_cursor_index(),
                filtered_current_list_.GetEntryTitle(get_cursor_index()), Notify);
        }
    }
}

void SongWheel::display_item(int32_t ListItem, int32_t ListPosition, float itemFraction)
{
	AABBd screen_box (0.0, 0.0, ScreenWidth, ScreenHeight);
	AABBd item_box = item_box_at(itemFraction);
	Vec2 pos(item_box.X1, item_box.Y1);
	// Vec2 size (item_box.width(), item_box.height()); 

    if (screen_box.Intersects(item_box))
    {
        bool IsSelected = false;
        std::shared_ptr<otoworm::ChartGroup> song = nullptr;
        std::string Text;

        if (ListItem != -1)
        {
            song = filtered_current_list_.GetSongEntry(ListItem);
            Text = filtered_current_list_.GetEntryTitle(ListItem);
            IsSelected = (ListPosition == selected_unbound_item_);
        }

        for (auto & Sprite : sprites_)
        {			
            Sprite.second->SetPosition(item_box.X1, item_box.Y1);

            if (transform_item)
                transform_item(Sprite.first, song, IsSelected, ListPosition);

            // Render the objects.
            Sprite.second->render();
        }

        for (auto & String : strings_)
        {
            String.second->SetPosition(pos);

            if (transform_string)
                transform_string(String.first, song, IsSelected, ListPosition, Text);

            String.second->render();
        }
    }
}


void SongWheel::render()
{
    int Index = get_cursor_index();
    std::unique_lock<std::mutex> lock(*m_load_mutex_);
    int Cur = 0;
    int Max = filtered_current_list_.GetNumEntries();

	// I only really need the top index.
	int DisplayEndIndex = display_start_index + display_item_count + 1;

	float TotalEntries = DisplayEndIndex - display_start_index;
    for (Cur = display_start_index; Cur <= DisplayEndIndex; Cur++)
    {
		// Cur*ItemHeight + shownListY

		float t = (Cur - display_start_index) / TotalEntries;
        if (Max)
        {
            int RealIndex = Cur % Max;

            while (RealIndex < 0) // Loop over..
                RealIndex += Max;

            display_item(RealIndex, Cur, t);
        }
        else
            display_item(-1, Cur, t);
    }
}

int32_t SongWheel::get_selected_item() const
{
    return selected_unbound_item_;
}


void SongWheel::set_selected_item(int32_t Item)
{
    if (!current_list_)
        return;

    selected_unbound_item_ = Item;

    // Get a bound item index
	if (get_num_items())
		Item = modulo(Item, get_num_items());
	else
		Item = 0;
    
    // Set bound item index to this.
    selected_bound_item_ = Item;
    on_song_tentative_select(GetSelectedChartGroup(), difficulty_index_);
}

int32_t SongWheel::index_at_point(float X, float Y)
{
	for (int cur = display_start_index;
		cur != display_start_index + display_item_count + 1;
		cur++) {
		float t = float(cur - display_start_index) / float(display_item_count + 1);

		if (item_box_at(t).IsInBox(X, Y))
			return cur;
	}

	return -1;
}

uint32_t SongWheel::normalized_index_at_point(float X, float Y)
{
    auto Idx = modulo(index_at_point(X, Y), get_num_items());
    return Idx;
}

int32_t SongWheel::get_num_items() const
{
    if (!current_list_)
        return 0;
    else
    {
        return filtered_current_list_.GetNumEntries();
    }
}

bool SongWheel::is_item_directory(int32_t Item) const
{
    if (filtered_current_list_.GetNumEntries())
    {
        while (Item < 0) Item += get_num_items();
        Item %= get_num_items();
        return filtered_current_list_.IsDirectory(Item);
    }

    return false;
}

void SongWheel::set_cursor_index(int Index)
{
    cursor_pos_ = Index;
}

void SongWheel::confirm_selection()
{
    if (!filtered_current_list_.IsDirectory(selected_bound_item_))
    {
		if (difficulty_index_ < GetSelectedChartGroup()->get_chart_count())
			on_song_confirm(GetSelectedChartGroup(), difficulty_index_);
    }
    else
    {
        current_list_ = current_list_->GetListEntry(selected_bound_item_).get();
		current_list_->SetInUse(true);

		reapply_filters();
        set_selected_item(selected_unbound_item_); // Update our selected item to new bounderies.
        on_song_tentative_select(GetSelectedChartGroup(), difficulty_index_);
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

void SongWheel::sort_by(ESortCriteria criteria)
{
	std::unique_lock<std::mutex> lock(*m_load_mutex_);
	list_root_->SortBy(criteria);
	reapply_filters();
}

void SongWheel::reapply_filters()
{
	if (!current_list_) return;

	filtered_current_list_.Clear();
	for (const auto& entry : current_list_->GetEntries()) {
		bool add = true;

		if (entry.Kind == ListEntry::Song) {
			auto song = std::static_pointer_cast<otoworm::ChartGroup>(entry.Data);
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
			filtered_current_list_.AddEntry(entry);
	}
}

void SongWheel::reset_filters()
{
	active_filters_.clear();
	reapply_filters();
}

void SongWheel::select_by(FuncFilterCriteria criteria)
{
	active_filters_.push_back(criteria);
	reapply_filters();
}
