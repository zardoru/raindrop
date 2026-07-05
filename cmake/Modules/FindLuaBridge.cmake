include(FetchContent)

FetchContent_Declare(LuaBridge
        GIT_REPOSITORY https://github.com/vinniefalco/LuaBridge
        GIT_TAG 2.10
        GIT_SHALLOW TRUE
        UPDATE_DISCONNECTED TRUE
)

FetchContent_MakeAvailable(LuaBridge)