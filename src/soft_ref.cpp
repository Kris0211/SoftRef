#include "soft_ref.hpp"

#include <classes/resource_loader.hpp>
#include <classes/resource_uid.hpp>

#include "soft_ref_loader.hpp"

VARIANT_ENUM_CAST(SoftRef::LoadState);

namespace godot {
    void SoftRef::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_path", "path"), &SoftRef::set_path);
        ClassDB::bind_method(D_METHOD("get_path"), &SoftRef::get_path);
        ClassDB::bind_method(D_METHOD("set_uid", "uid"), &SoftRef::set_uid);
        ClassDB::bind_method(D_METHOD("get_uid"), &SoftRef::get_uid);
        ClassDB::bind_method(D_METHOD("get"), &SoftRef::get);
        ClassDB::bind_method(D_METHOD("get_load_state"), &SoftRef::get_load_state);
        ClassDB::bind_method(D_METHOD("get_progress"), &SoftRef::get_progress);
        ClassDB::bind_method(D_METHOD("is_valid"), &SoftRef::is_valid);
        ClassDB::bind_method(D_METHOD("is_null"), &SoftRef::is_null);
        ClassDB::bind_method(D_METHOD("is_loaded"), &SoftRef::is_loaded);
        ClassDB::bind_method(D_METHOD("is_pending"), &SoftRef::is_pending);
        ClassDB::bind_method(D_METHOD("is_failed"), &SoftRef::is_failed);
        ClassDB::bind_method(D_METHOD("load_sync"), &SoftRef::load_sync);
        ClassDB::bind_method(D_METHOD("load_async"), &SoftRef::load_async);
        ClassDB::bind_method(D_METHOD("reset"), &SoftRef::reset);

        ADD_SIGNAL(MethodInfo("loading_finished"));

        BIND_ENUM_CONSTANT(LOADED);
        BIND_ENUM_CONSTANT(EMPTY);
        BIND_ENUM_CONSTANT(NOT_LOADED);
        BIND_ENUM_CONSTANT(PENDING);
        BIND_ENUM_CONSTANT(FAILED);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "path"), "set_path", "get_path");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "uid"), "set_uid", "get_uid");
    }

    SoftRef::SoftRef(const String &p_path, bool path_as_uid) {
        if (path_as_uid)
            set_uid(p_path);
        else
            set_path(p_path);
    }

    void SoftRef::set_path(const String &p_path) {
        path = p_path;
        uid = ResourceUID::path_to_uid(path); // Returns the unchanged path if it has no associated UID.
        if (uid.is_empty() || path == uid)
        {
            WARN_PRINT(vformat(
                "SoftRef: UID for resource at path '%s' is invalid or does not exist.", path));
        }
    }

    String SoftRef::get_path() const {
        return path;
    }

    void SoftRef::set_uid(const String &p_uid) {
        uid = p_uid;
        path = ResourceUID::uid_to_path(p_uid);
    }

    String SoftRef::get_uid() const {
        return uid;
    }

    bool SoftRef::has_path() const {
        return !path.is_empty();
    }

    bool SoftRef::has_uid() const {
        return !uid.is_empty();
    }

    Ref<Resource> SoftRef::get() const {
        return resource;
    }

    bool SoftRef::is_valid() const {
        return resource.is_valid();
    }

    bool SoftRef::is_null() const {
        return resource.is_null();
    }

    bool SoftRef::is_loaded() const {
        return resource.is_valid() && load_state == LOADED;
    }

    bool SoftRef::is_pending() const {
        return load_state == PENDING;
    }

    bool SoftRef::is_failed() const {
        return load_state == FAILED;
    }

    float SoftRef::get_progress() const {
        if (is_loaded()) {
            return 1.f;
        }

        if (load_state != PENDING) {
            return 0.f;
        }

        TypedArray<float> ratio;
        ratio.resize(1);

        ResourceLoader::get_singleton()->load_threaded_get_status(path, ratio);
        return ratio.is_empty() ? 0.f :  static_cast<float>(ratio[0]);
    }

    SoftRef::LoadState SoftRef::get_load_state() const {
        return load_state;
    }

    StringName SoftRef::get_load_state_name() const {
        static const StringName LOADED_NAME("Loaded");
        static const StringName EMPTY_NAME("Empty");
        static const StringName NOT_LOADED_NAME("Not Loaded");
        static const StringName PENDING_NAME("Pending");
        static const StringName FAILED_NAME("Failed");
        static const StringName UNKNOWN_NAME("Unknown");

        switch (load_state) {
            case LOADED:
                return LOADED_NAME;
            case EMPTY:
                return EMPTY_NAME;
            case NOT_LOADED:
                return NOT_LOADED_NAME;
            case PENDING:
                return PENDING_NAME;
            case FAILED:
                return FAILED_NAME;
            default:
                return UNKNOWN_NAME;
        }
    }

    Ref<Resource> SoftRef::load_sync() {
        if (this->is_loaded()) {
            emit_signal("loading_finished");
            return resource;
        }

        if (path.is_empty()) {
            ERR_PRINT("SoftRef: Cannot load resource from empty path.");
            load_state = EMPTY;
            resource = Ref<Resource>();
            emit_signal("loading_finished");
            return resource;
        }

        if (this->get_load_state() == PENDING) {
            const Ref<Resource> res = ResourceLoader::get_singleton()->load_threaded_get(path);
            if (res.is_valid()) {
                resource = res;
                load_state = LOADED;
            }
            else {
                load_state = FAILED;
                ERR_PRINT(vformat("SoftRef: Failed to complete threaded load for %s.", path));
            }

            emit_signal("loading_finished");
            return resource;
        }

        const Ref<Resource> loaded = ResourceLoader::get_singleton()->load(path);
        if (loaded.is_valid()) {
            resource = loaded;
            load_state = LOADED;
        }
        else {
            ERR_PRINT(vformat("SoftRef: Failed to load resource at path: %s.", path));
            resource = Ref<Resource>();
            load_state = FAILED;
        }

        emit_signal("loading_finished");
        return resource;
    }

    void SoftRef::load_async() {
        if (this->is_loaded()) {
            WARN_PRINT("SoftRef: Resource already loaded.");
            emit_signal("loading_finished");
            return;
        }

        if (this->is_pending()) {
            WARN_PRINT("SoftRef: Resource is already being loaded.");
            return;
        }

        if (path.is_empty()) {
            ERR_PRINT("SoftRef: Cannot load resource from empty path.");
            emit_signal("loading_finished");
            load_state = EMPTY;
            return;
        }

        SoftRefLoader::get_singleton()->request_load(this);
    }

    void SoftRef::reset() {
        resource = Ref<Resource>();
        load_state = EMPTY;
    }

    String SoftRef::_to_string() const {
        const String res_info = resource.is_valid() ? resource->to_string() : "null";
        return vformat("<SoftRef#%s>(%s - Resource: %s)",
            itos(get_instance_id()), get_load_state_name(), res_info);
    }
} // godot
