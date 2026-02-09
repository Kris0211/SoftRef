#ifndef SOTF_REF_LOADER_H
#define SOTF_REF_LOADER_H

#include <string>
#include <classes/object.hpp>
#include <classes/scene_tree.hpp>

#include "soft_ref.hpp"

namespace godot {

struct PendingLoad {
    Vector<Ref<SoftRef>> soft_refs;
    String path;
};

class SoftRefLoader : public Object {
    GDCLASS(SoftRefLoader, Object)

private:
    static SoftRefLoader *singleton;
    Vector<PendingLoad> pending_refs;
    bool initialized = false;

public:
    SoftRefLoader();

    static SoftRefLoader* get_singleton() { return singleton; }

    void _poll_loading();

    void request_load(SoftRef* soft_ref);
    Ref<SoftRef> request_load_from_path(const String& path);
    Ref<SoftRef> request_load_from_uid(const String& uid);

protected:
    static void _bind_methods();
};
}

#endif //SOTF_REF_LOADER_H
