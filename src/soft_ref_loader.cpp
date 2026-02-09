#include "soft_ref_loader.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

namespace godot {
    SoftRefLoader *SoftRefLoader::singleton = nullptr;

    void SoftRefLoader::_bind_methods() {
        ClassDB::bind_method(D_METHOD("_poll_loading" ), &SoftRefLoader::_poll_loading);
        ClassDB::bind_method(D_METHOD("request_load", "soft_ref"), &SoftRefLoader::request_load);
        ClassDB::bind_method(D_METHOD("request_load_from_path", "path"), &SoftRefLoader::request_load_from_path);
        ClassDB::bind_method(D_METHOD("request_load_from_uid", "uid"), &SoftRefLoader::request_load_from_uid);
    }

    SoftRefLoader::SoftRefLoader() {
        singleton = this;
    }

    void SoftRefLoader::_poll_loading() {
        if (pending_refs.is_empty()) {
            return;
        }

        Vector<int64_t> finished;

        for (int64_t i = pending_refs.size() - 1; i >= 0; --i) {
            PendingLoad &pending = pending_refs.write[i];
            if (pending.soft_refs.is_empty()) {
                pending_refs.remove_at(i);
                continue;
            }

            for (int64_t k = pending.soft_refs.size() - 1; k >= 0; --k) {
                Ref<SoftRef> &ref = pending.soft_refs.write[k];
                if (!ref.is_valid() || ref->is_loaded()) {
                    pending.soft_refs.remove_at(k);
                    continue;
                }

                const String& path = pending.path;
                Ref<Resource> res;
                const ResourceLoader::ThreadLoadStatus status =
                            ResourceLoader::get_singleton()->load_threaded_get_status(path);

                switch (status) {
                    case ResourceLoader::ThreadLoadStatus::THREAD_LOAD_IN_PROGRESS:
                        // Still loading
                        break;

                    case ResourceLoader::ThreadLoadStatus::THREAD_LOAD_LOADED:
                        res = ResourceLoader::get_singleton()->load_threaded_get(path);
                        if (res.is_valid()) {
                            ref->set_resource(res);
                            ref->set_load_state(SoftRef::LoadState::LOADED);
                        }
                        else {
                            ref->set_load_state(SoftRef::LoadState::FAILED);
                            ERR_PRINT(vformat("SoftRefLoader: Failed to load resource at path %s.", path));
                        }
                        ref->emit_signal("loading_finished");
                        pending.soft_refs.remove_at(k);
                        break;

                    case ResourceLoader::ThreadLoadStatus::THREAD_LOAD_FAILED:
                    case ResourceLoader::ThreadLoadStatus::THREAD_LOAD_INVALID_RESOURCE:
                    default:
                        ref->set_load_state(SoftRef::LoadState::FAILED);
                        ERR_PRINT(vformat("SoftRefLoader: Failed to load resource at path %s.", path));
                        ref->emit_signal("loading_finished");
                        pending.soft_refs.remove_at(k);
                        break;
                }
            }

            if (pending.soft_refs.is_empty()) {
                pending_refs.remove_at(i);
            }
        }
    }

    void SoftRefLoader::request_load(SoftRef* soft_ref) {
        if (soft_ref == nullptr) {
            return;
        }

        if (soft_ref->path.is_empty()) {
            ERR_PRINT("SoftRef: Cannot load resource from empty path.");
            soft_ref->set_load_state(SoftRef::EMPTY);
            soft_ref->emit_signal("loading_finished");
            return;
        }

        if (soft_ref->is_loaded()) {
            WARN_PRINT("SoftRef: Resource already loaded.");
            soft_ref->emit_signal("loading_finished");
            return;
        }

        if (soft_ref->is_pending()) {
            WARN_PRINT("SoftRef: Resource is already being loaded.");
            return;
        }

        if (!initialized) {
            if (SceneTree* tree = cast_to<SceneTree>(Engine::get_singleton()->get_main_loop())) {
                tree->connect("process_frame", Callable(singleton, "_poll_loading"));
                initialized = true;
            }
            else {
                ERR_PRINT("SoftRefLoader: Could not connect to SceneTree!");
                soft_ref->set_load_state(SoftRef::FAILED);
                soft_ref->emit_signal("loading_finished");
                return;
            }
        }

        for (PendingLoad &pending : pending_refs) {
            if (pending.path == soft_ref->path) {
                soft_ref->set_load_state(SoftRef::PENDING);
                pending.soft_refs.push_back(Ref<SoftRef>(soft_ref));
                return;
            }
        }

        soft_ref->set_load_state(SoftRef::PENDING);
        const Error err = ResourceLoader::get_singleton()->load_threaded_request(soft_ref->get_path());
        if (err != OK) {
            ERR_PRINT("SoftRefLoader: Failed to request threaded load.");
            soft_ref->set_load_state(SoftRef::LoadState::FAILED);
            soft_ref->emit_signal("loading_finished");
            return;
        }

        pending_refs.push_back({
            {Ref<SoftRef>(soft_ref)},
            soft_ref->get_path()
        });
    }

    Ref<SoftRef> SoftRefLoader::request_load_from_path(const String &path) {
        const Ref<SoftRef> soft_ref = memnew(SoftRef(path, false));
        call_deferred("request_load", soft_ref.ptr());
        return soft_ref;
    }

    Ref<SoftRef> SoftRefLoader::request_load_from_uid(const String &uid) {
        const Ref<SoftRef> soft_ref = memnew(SoftRef(uid, true));
        call_deferred("request_load", soft_ref.ptr());
        return soft_ref;
    }
}
