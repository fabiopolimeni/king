0. C++11 assumed available because your implementation already uses unique_ptr
1. OpenGL 4.3, supported and extendible
2. Resources are simple uint32_t don't need a Pimpl implementation
3. Where distructors are not virtual, the class declaration is by design considered final (not extendible)
4. Use a sprite batch factory that will potentially allow to render all the sprites with one draw call
5. Uses of sprite template because all sprites have the same vertex layout, and they only changes in the texture cooridnates