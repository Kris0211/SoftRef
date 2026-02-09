#ifndef SOFT_REF_H
#define SOFT_REF_H

#include <classes/ref_counted.hpp>
#include <classes/resource.hpp>

namespace godot {

class SoftRef : public RefCounted {
    GDCLASS(SoftRef, RefCounted);

    friend class SoftRefLoader;

public:
    enum LoadState {
        /// Loaded and valid
        LOADED = 0,
        /// No resource path assigned
        EMPTY = 1,
        /// Has resource path, but dit not load any resource yet
        NOT_LOADED = 2,
        /// Async request in progress
        PENDING = 3,
        /// Load attempt failed
        FAILED = 4
    };

private:
    Ref<Resource> resource;
    String path;
    String uid;
    LoadState load_state = EMPTY;

public:
    bool operator==(const SoftRef &other) const { return path == other.path; }
    bool operator!=(const SoftRef &other) const { return path != other.path; }

protected:
    static void _bind_methods();

public:
    SoftRef() = default;
    explicit SoftRef(const String& p_path, bool path_as_uid = false);

    void set_path(const String &p_path);
    String get_path() const;

    void set_uid(const String &p_uid);
    String get_uid() const;

    bool has_path() const;
    bool has_uid() const;

    Ref<Resource> get() const;

    bool is_valid() const;
    bool is_null() const;
    bool is_loaded() const;
    bool is_pending() const;
    bool is_failed() const;

    float get_progress() const;

    LoadState get_load_state() const;
    StringName get_load_state_name() const;

    Ref<Resource> load_sync();
    void load_async();

    void reset();

    String _to_string() const;

private:
    void set_resource(const Ref<Resource> &p_resource) { this->resource = p_resource; }
    void set_load_state(const LoadState p_load_state) { this->load_state = p_load_state; }
};

} // godot

#endif //SOFT_REF_H
