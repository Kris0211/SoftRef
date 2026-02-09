#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H
#include <godot_cpp/godot.hpp>

using namespace godot;

void initialize_softref_module(ModuleInitializationLevel p_level);
void uninitialize_softref_module(ModuleInitializationLevel p_level);

#endif // REGISTER_TYPES_H
