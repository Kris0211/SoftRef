#include <godot_cpp/godot.hpp>
#include <godot_cpp/classes/engine.hpp>
#include "soft_ref.hpp"
#include "soft_ref_loader.hpp"

using namespace godot;


void initialize_softref_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    ClassDB::register_class<SoftRef>();
    ClassDB::register_class<SoftRefLoader>();
    SoftRefLoader* loader = memnew(SoftRefLoader);
    Engine::get_singleton()->register_singleton("SoftRefLoader", loader);
}

void uninitialize_softref_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    if (SoftRefLoader* loader = Object::cast_to<SoftRefLoader>(
        Engine::get_singleton()->get_singleton("SoftRefLoader"))) {
        Engine::get_singleton()->unregister_singleton("SoftRefLoader");
        memdelete<SoftRefLoader>(loader);
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT softref_library_init(
        GDExtensionInterfaceGetProcAddress p_get_proc_address,
        const GDExtensionClassLibraryPtr p_library,
        GDExtensionInitialization *r_initialization
    ) {
        static GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
        init_obj.register_initializer(initialize_softref_module);
        init_obj.register_terminator(uninitialize_softref_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
        return init_obj.init();
    }
}
