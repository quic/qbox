# namespace `sc_core`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# class `Router` {#class_router}

```
class Router
  : private sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`struct `[``initiator_info``](#struct_router_1_1initiator__info)        | 
`struct `[``target_info``](#struct_router_1_1target__info)        | 
`public inline  explicit Router(const sc_core::sc_module_name & nm)` | 
`public  Router() = delete` | 
`public  Router(const `[`Router`](#class_router)` &) = delete` | 
`public inline  ~Router()` | 
`public inline void add_target(target & t,uint64_t address,uint64_t size)` | 
`public inline virtual void add_initiator(initiator & i)` | 
`protected inline virtual void before_end_of_elaboration()` | 

## Members

### `struct `[``initiator_info``](#struct_router_1_1initiator__info) {#struct_router_1_1initiator__info}




### `struct `[``target_info``](#struct_router_1_1target__info) {#struct_router_1_1target__info}




### `public inline  explicit Router(const sc_core::sc_module_name & nm)` {#class_router_1a86b4f8ee923f37c29ca2761c6fa2e606}





### `public  Router() = delete` {#class_router_1a2e393ddacf6f37388c70314c575b6ec7}





### `public  Router(const `[`Router`](#class_router)` &) = delete` {#class_router_1a169555c2ce3699814909265c0afccd69}





### `public inline  ~Router()` {#class_router_1a02ffa88bba81796c7c54f1a373189d0f}





### `public inline void add_target(target & t,uint64_t address,uint64_t size)` {#class_router_1a580bc271bf2eb258e3c98cda69493a3b}





### `public inline virtual void add_initiator(initiator & i)` {#class_router_1a61a47b45a1ab255fb7e1a7b376843c73}





### `protected inline virtual void before_end_of_elaboration()` {#class_router_1afc73fcb7544f50aaf806654d98426b59}






# struct `initiator_info` {#struct_router_1_1initiator__info}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public size_t index` | 
`public initiator * i` | 

## Members

### `public size_t index` {#struct_router_1_1initiator__info_1a4db02e82bea93e9edbb5cad7b4fbf5f7}





### `public initiator * i` {#struct_router_1_1initiator__info_1a8e84c34c5ee30707c54da1c05461fea0}






# struct `target_info` {#struct_router_1_1target__info}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public size_t index` | 
`public target * t` | 
`public sc_dt::uint64 address` | 
`public sc_dt::uint64 size` | 

## Members

### `public size_t index` {#struct_router_1_1target__info_1a42f99795ef4e313f90a14a49b3483134}





### `public target * t` {#struct_router_1_1target__info_1aa4db22b7c53aa115ebcbf188c4edd6ce}





### `public sc_dt::uint64 address` {#struct_router_1_1target__info_1adbe8bd3d09eef6b46cb76e087b04df3c}





### `public sc_dt::uint64 size` {#struct_router_1_1target__info_1af06e1c8b548d2543df81b07759f14c94}






# class `Memory` {#class_memory}

```
class Memory
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public tlm_utils::simple_target_socket< `[`Memory`](#class_memory)` > socket` | 
`public inline  Memory(sc_core::sc_module_name name,uint64_t size)` | 
`public  Memory() = delete` | 
`public  Memory(const `[`Memory`](#class_memory)` &) = delete` | 
`public inline  ~Memory()` | 
`public inline uint64_t size()` | 
`public inline size_t load(std::string filename,uint64_t addr)` | 
`public inline void load(const uint8_t * ptr,uint64_t len,uint64_t addr)` | 

## Members

### `public tlm_utils::simple_target_socket< `[`Memory`](#class_memory)` > socket` {#class_memory_1a4d01fe2a65b1eebf210b7f94357e6919}





### `public inline  Memory(sc_core::sc_module_name name,uint64_t size)` {#class_memory_1aaebc270c5271842172cad510ade1449f}





### `public  Memory() = delete` {#class_memory_1a113d215ace67c2ed2366a091fb28c17c}





### `public  Memory(const `[`Memory`](#class_memory)` &) = delete` {#class_memory_1a99380a9fcf78020c641ee9c9274b912c}





### `public inline  ~Memory()` {#class_memory_1a0ffa9759ebbf103f11132a505b93bdc0}





### `public inline uint64_t size()` {#class_memory_1aa4ae54abea1d5f7d653d5024a8a3b52e}





### `public inline size_t load(std::string filename,uint64_t addr)` {#class_memory_1a14bcd79e72b888fc101fb3781816da12}





### `public inline void load(const uint8_t * ptr,uint64_t len,uint64_t addr)` {#class_memory_1ae05ff4da5c199cb278bc1a49619299fc}






