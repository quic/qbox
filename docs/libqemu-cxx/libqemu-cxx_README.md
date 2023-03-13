# namespace `qemu`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[``ArmNvic``](#classqemu_1_1_arm_nvic)    | 
`class `[``Bus``](#classqemu_1_1_bus)    | 
`class `[``Chardev``](#classqemu_1_1_chardev)    | 
`class `[``Cpu``](#classqemu_1_1_cpu)    | 
`class `[``CpuAarch64``](#classqemu_1_1_cpu_aarch64)    | 
`class `[``CpuArm``](#classqemu_1_1_cpu_arm)    | 
`class `[``CpuMicroblaze``](#classqemu_1_1_cpu_microblaze)    | 
`class `[``CpuRiscv``](#classqemu_1_1_cpu_riscv)    | 
`class `[``CpuRiscv32``](#classqemu_1_1_cpu_riscv32)    | 
`class `[``CpuRiscv64``](#classqemu_1_1_cpu_riscv64)    | 
`class `[``Device``](#classqemu_1_1_device)    | 
`class `[``Gpio``](#classqemu_1_1_gpio)    | 
`class `[``InvalidLibraryException``](#classqemu_1_1_invalid_library_exception)    | 
`class `[``LibQemu``](#classqemu_1_1_lib_qemu)    | 
`class `[``LibQemuException``](#classqemu_1_1_lib_qemu_exception)    | 
`class `[``LibQemuInternals``](#classqemu_1_1_lib_qemu_internals)    | 
`class `[``LibQemuObjectCallback``](#classqemu_1_1_lib_qemu_object_callback)    | 
`class `[``LibQemuObjectCallbackBase``](#classqemu_1_1_lib_qemu_object_callback_base)    | 
`class `[``LibraryIface``](#classqemu_1_1_library_iface)    | 
`class `[``LibraryLoaderIface``](#classqemu_1_1_library_loader_iface)    | 
`class `[``LibraryLoadErrorException``](#classqemu_1_1_library_load_error_exception)    | 
`class `[``MemoryRegion``](#classqemu_1_1_memory_region)    | 
`class `[``MemoryRegionOps``](#classqemu_1_1_memory_region_ops)    | 
`class `[``Object``](#classqemu_1_1_object)    | 
`class `[``SetPropertyException``](#classqemu_1_1_set_property_exception)    | 
`class `[``SysBusDevice``](#classqemu_1_1_sys_bus_device)    | 
`class `[``TargetNotSupportedException``](#classqemu_1_1_target_not_supported_exception)    | 
`class `[``Timer``](#classqemu_1_1_timer)    | 

# class `ArmNvic` {#classqemu_1_1_arm_nvic}

```
class ArmNvic
  : public qemu::Device
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  ArmNvic() = default` | 
`public  ArmNvic(const `[`ArmNvic`](#classqemu_1_1_arm_nvic)` &) = default` | 
`public inline  ArmNvic(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void add_cpu_link()` | 

## Members

### `public  ArmNvic() = default` {#classqemu_1_1_arm_nvic_1a5a8fb2a8cac21864bf05c04b8aeda2cb}





### `public  ArmNvic(const `[`ArmNvic`](#classqemu_1_1_arm_nvic)` &) = default` {#classqemu_1_1_arm_nvic_1af8719152b5f47127bf4ee3829ffc2ec8}





### `public inline  ArmNvic(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_arm_nvic_1a09c18a893c06af465912f01161d7f4de}





### `public void add_cpu_link()` {#classqemu_1_1_arm_nvic_1a8f747fc5d9319bbc3cacb981e343d92f}






# class `Bus` {#classqemu_1_1_bus}

```
class Bus
  : public qemu::Object
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Bus() = default` | 
`public  Bus(const `[`Bus`](#classqemu_1_1_bus)` & o) = default` | 
`public inline  Bus(const `[`Object`](#classqemu_1_1_object)` & o)` | 

## Members

### `public  Bus() = default` {#classqemu_1_1_bus_1a3d96558a58a771fc7c74e5f87e20d742}





### `public  Bus(const `[`Bus`](#classqemu_1_1_bus)` & o) = default` {#classqemu_1_1_bus_1adf53833fdf42e75588250d545ff95992}





### `public inline  Bus(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_bus_1a54f8ea5646fd9cbfd823b7c0aeb09f10}






# class `Chardev` {#classqemu_1_1_chardev}

```
class Chardev
  : public qemu::Object
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Chardev() = default` | 
`public  Chardev(const `[`Chardev`](#classqemu_1_1_chardev)` & o) = default` | 
`public inline  Chardev(const `[`Object`](#classqemu_1_1_object)` & o)` | 

## Members

### `public  Chardev() = default` {#classqemu_1_1_chardev_1ae101879971b52f604ab9d51e77e23b25}





### `public  Chardev(const `[`Chardev`](#classqemu_1_1_chardev)` & o) = default` {#classqemu_1_1_chardev_1a6b94b186d6d314cc6b1e64bb0be6d98e}





### `public inline  Chardev(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_chardev_1a0151876a72363d17d845efdb598c1943}






# class `Cpu` {#classqemu_1_1_cpu}

```
class Cpu
  : public qemu::Device
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Cpu() = default` | 
`public  Cpu(const `[`Cpu`](#classqemu_1_1_cpu)` &) = default` | 
`public inline  Cpu(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void loop()` | 
`public bool loop_is_busy()` | 
`public bool can_run()` | 
`public void set_soft_stopped(bool stopped)` | 
`public void halt(bool halted)` | 
`public void reset()` | 
`public void set_unplug(bool unplug)` | 
`public void remove_sync()` | 
`public void register_thread()` | 
`public `[`Cpu`](#classqemu_1_1_cpu)` set_as_current()` | 
`public void kick()` | 
`public void async_safe_run(AsyncJobFn job,void * arg)` | 
`public void set_end_of_loop_callback(EndOfLoopCallbackFn cb)` | 
`public void set_kick_callback(CpuKickCallbackFn cb)` | 

## Members

### `public  Cpu() = default` {#classqemu_1_1_cpu_1a19a49e962ec320e32442493204980a4a}





### `public  Cpu(const `[`Cpu`](#classqemu_1_1_cpu)` &) = default` {#classqemu_1_1_cpu_1a5e05084ac2fe739f814a36f530737594}





### `public inline  Cpu(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_1a69b2107134dcf57799dcf3797559f8e7}





### `public void loop()` {#classqemu_1_1_cpu_1a01f5d8739d110580ee2bd73a3b11a330}





### `public bool loop_is_busy()` {#classqemu_1_1_cpu_1a94fc36c745b373f4c19c948b00520d03}





### `public bool can_run()` {#classqemu_1_1_cpu_1a129ca8c042857fe5bae675f56fab2c82}





### `public void set_soft_stopped(bool stopped)` {#classqemu_1_1_cpu_1a061e7b14bd3a79403165c7dfca33531a}





### `public void halt(bool halted)` {#classqemu_1_1_cpu_1afeb3493f659ddb1fc821e9c8c70f7df9}





### `public void reset()` {#classqemu_1_1_cpu_1ae566936b2d95c0a17cb9880bbba53d5d}





### `public void set_unplug(bool unplug)` {#classqemu_1_1_cpu_1a5d9072d7d0920a4b2f7e8bb4abdca6f9}





### `public void remove_sync()` {#classqemu_1_1_cpu_1ae7654a1e6157f950743050a5eeef4a14}





### `public void register_thread()` {#classqemu_1_1_cpu_1ab8e4c91099cca248ea6b61d4b3dc73c6}





### `public `[`Cpu`](#classqemu_1_1_cpu)` set_as_current()` {#classqemu_1_1_cpu_1a1c075142a0b031020d2e8890f00bf6e5}





### `public void kick()` {#classqemu_1_1_cpu_1a830c49af0996b22f1f0d7649917958a0}





### `public void async_safe_run(AsyncJobFn job,void * arg)` {#classqemu_1_1_cpu_1abe811dd1947034448a56519e7d226182}





### `public void set_end_of_loop_callback(EndOfLoopCallbackFn cb)` {#classqemu_1_1_cpu_1a16c8030eb2f732f67263de259045ea0b}





### `public void set_kick_callback(CpuKickCallbackFn cb)` {#classqemu_1_1_cpu_1a038ddb5c92af591f6bd3ffbccac5ca35}






# class `CpuAarch64` {#classqemu_1_1_cpu_aarch64}

```
class CpuAarch64
  : public qemu::CpuArm
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  CpuAarch64() = default` | 
`public  CpuAarch64(const `[`CpuAarch64`](#classqemu_1_1_cpu_aarch64)` &) = default` | 
`public inline  CpuAarch64(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void set_aarch64_mode(bool aarch64_mode)` | 

## Members

### `public  CpuAarch64() = default` {#classqemu_1_1_cpu_aarch64_1a6c883c3fcf9d343f03b9b56e9eb0a3e5}





### `public  CpuAarch64(const `[`CpuAarch64`](#classqemu_1_1_cpu_aarch64)` &) = default` {#classqemu_1_1_cpu_aarch64_1a0e7f376add4026ab36048a42385313cf}





### `public inline  CpuAarch64(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_aarch64_1a34ff4c94c2f3c76914e9d902673af7df}





### `public void set_aarch64_mode(bool aarch64_mode)` {#classqemu_1_1_cpu_aarch64_1a2ade02d3c6d14d34f2f09d6d9f397257}






# class `CpuArm` {#classqemu_1_1_cpu_arm}

```
class CpuArm
  : public qemu::Cpu
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  CpuArm() = default` | 
`public  CpuArm(const `[`CpuArm`](#classqemu_1_1_cpu_arm)` &) = default` | 
`public inline  CpuArm(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void set_cp15_cbar(uint64_t cbar)` | 
`public void add_nvic_link()` | 
`public uint64_t get_exclusive_val()` | 

## Members

### `public  CpuArm() = default` {#classqemu_1_1_cpu_arm_1a6a6893fe6d21021d1768b778795131b6}





### `public  CpuArm(const `[`CpuArm`](#classqemu_1_1_cpu_arm)` &) = default` {#classqemu_1_1_cpu_arm_1a6c4d8dc36ee62e4348ffb12c36c4348f}





### `public inline  CpuArm(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_arm_1a000b4dd20afc997c8bfebb3d0c16de24}





### `public void set_cp15_cbar(uint64_t cbar)` {#classqemu_1_1_cpu_arm_1a059e33983518795463d77b438134bf71}





### `public void add_nvic_link()` {#classqemu_1_1_cpu_arm_1af5c8568a464f1ac53e74a086cbdb5ffc}





### `public uint64_t get_exclusive_val()` {#classqemu_1_1_cpu_arm_1a498638c8d1db4c71f90075eaf2b14500}






# class `CpuMicroblaze` {#classqemu_1_1_cpu_microblaze}

```
class CpuMicroblaze
  : public qemu::Cpu
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  CpuMicroblaze() = default` | 
`public  CpuMicroblaze(const `[`CpuMicroblaze`](#classqemu_1_1_cpu_microblaze)` &) = default` | 
`public inline  CpuMicroblaze(const `[`Object`](#classqemu_1_1_object)` & o)` | 

## Members

### `public  CpuMicroblaze() = default` {#classqemu_1_1_cpu_microblaze_1a5974eb661bfc6c61e3e03c611e162171}





### `public  CpuMicroblaze(const `[`CpuMicroblaze`](#classqemu_1_1_cpu_microblaze)` &) = default` {#classqemu_1_1_cpu_microblaze_1ac66f1eb97a305768574ee2f49dd8c1e0}





### `public inline  CpuMicroblaze(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_microblaze_1a2a72652bf7d60ed2121ec844034d3933}






# class `CpuRiscv` {#classqemu_1_1_cpu_riscv}

```
class CpuRiscv
  : public qemu::Cpu
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  CpuRiscv() = default` | 
`public  CpuRiscv(const `[`CpuRiscv`](#classqemu_1_1_cpu_riscv)` &) = default` | 
`public inline  CpuRiscv(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void set_mip_update_callback(MipUpdateCallbackFn cb)` | 

## Members

### `public  CpuRiscv() = default` {#classqemu_1_1_cpu_riscv_1a1d62681ee448e2068a5f51b0b6c72db0}





### `public  CpuRiscv(const `[`CpuRiscv`](#classqemu_1_1_cpu_riscv)` &) = default` {#classqemu_1_1_cpu_riscv_1acad3bf4c5187bd3e96911a3cd4537bf1}





### `public inline  CpuRiscv(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_riscv_1ae50c0c187967d962434f486864840861}





### `public void set_mip_update_callback(MipUpdateCallbackFn cb)` {#classqemu_1_1_cpu_riscv_1a9c84fa3d1f7173f3b25d99581c79bf48}






# class `CpuRiscv32` {#classqemu_1_1_cpu_riscv32}

```
class CpuRiscv32
  : public qemu::CpuRiscv
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  CpuRiscv32() = default` | 
`public  CpuRiscv32(const `[`CpuRiscv32`](#classqemu_1_1_cpu_riscv32)` &) = default` | 
`public inline  CpuRiscv32(const `[`Object`](#classqemu_1_1_object)` & o)` | 

## Members

### `public  CpuRiscv32() = default` {#classqemu_1_1_cpu_riscv32_1ac88b18edbbe662ed5f7e6a5993213f13}





### `public  CpuRiscv32(const `[`CpuRiscv32`](#classqemu_1_1_cpu_riscv32)` &) = default` {#classqemu_1_1_cpu_riscv32_1a8d8daef13953a109449131f1f98207b5}





### `public inline  CpuRiscv32(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_riscv32_1a708edc0e5d7c2d333479d8705820e48a}






# class `CpuRiscv64` {#classqemu_1_1_cpu_riscv64}

```
class CpuRiscv64
  : public qemu::CpuRiscv
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  CpuRiscv64() = default` | 
`public  CpuRiscv64(const `[`CpuRiscv64`](#classqemu_1_1_cpu_riscv64)` &) = default` | 
`public inline  CpuRiscv64(const `[`Object`](#classqemu_1_1_object)` & o)` | 

## Members

### `public  CpuRiscv64() = default` {#classqemu_1_1_cpu_riscv64_1a23209bc154d89a074b4f9bd721505a77}





### `public  CpuRiscv64(const `[`CpuRiscv64`](#classqemu_1_1_cpu_riscv64)` &) = default` {#classqemu_1_1_cpu_riscv64_1adb081063db1d607da310e3cddaf3ae4c}





### `public inline  CpuRiscv64(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_cpu_riscv64_1ab8ff8ef65d851d67686b710d555aedfc}






# class `Device` {#classqemu_1_1_device}

```
class Device
  : public qemu::Object
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Device() = default` | 
`public  Device(const `[`Device`](#classqemu_1_1_device)` &) = default` | 
`public inline  Device(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void connect_gpio_out(int idx,`[`Gpio`](#classqemu_1_1_gpio)` gpio)` | 
`public void connect_gpio_out_named(const char * name,int idx,`[`Gpio`](#classqemu_1_1_gpio)` gpio)` | 
`public `[`Gpio`](#classqemu_1_1_gpio)` get_gpio_in(int idx)` | 
`public `[`Gpio`](#classqemu_1_1_gpio)` get_gpio_in_named(const char * name,int idx)` | 
`public `[`Bus`](#classqemu_1_1_bus)` get_child_bus(const char * name)` | 
`public void set_parent_bus(`[`Bus`](#classqemu_1_1_bus)` bus)` | 
`public void set_prop_chardev(const char * name,`[`Chardev`](#classqemu_1_1_chardev)` chr)` | 

## Members

### `public  Device() = default` {#classqemu_1_1_device_1a5b0cf63b1e6ff4fa0440ec6ac95f5d62}





### `public  Device(const `[`Device`](#classqemu_1_1_device)` &) = default` {#classqemu_1_1_device_1a2691247be2aaaa4d6d79b32678df3111}





### `public inline  Device(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_device_1afcbf0bb9a32daf36debc08af226d4f92}





### `public void connect_gpio_out(int idx,`[`Gpio`](#classqemu_1_1_gpio)` gpio)` {#classqemu_1_1_device_1afe9f88b613b1d0cab4480c13f9fd3685}





### `public void connect_gpio_out_named(const char * name,int idx,`[`Gpio`](#classqemu_1_1_gpio)` gpio)` {#classqemu_1_1_device_1a0a43a5cee445707b6ffda779e57d25b3}





### `public `[`Gpio`](#classqemu_1_1_gpio)` get_gpio_in(int idx)` {#classqemu_1_1_device_1a40cb47bb168e36e995ec217ab74cea93}





### `public `[`Gpio`](#classqemu_1_1_gpio)` get_gpio_in_named(const char * name,int idx)` {#classqemu_1_1_device_1ae4afaa2ba79268150dc54f7ad6c1c28a}





### `public `[`Bus`](#classqemu_1_1_bus)` get_child_bus(const char * name)` {#classqemu_1_1_device_1a8345cdef9566950f2c679a715f3d1cfc}





### `public void set_parent_bus(`[`Bus`](#classqemu_1_1_bus)` bus)` {#classqemu_1_1_device_1aef2450c9707d48fefe96b88236443430}





### `public void set_prop_chardev(const char * name,`[`Chardev`](#classqemu_1_1_chardev)` chr)` {#classqemu_1_1_device_1ade9c8cfeec77bdb255283585b6f2cf2b}






# class `Gpio` {#classqemu_1_1_gpio}

```
class Gpio
  : public qemu::Object
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[``GpioProxy``](#classqemu_1_1_gpio_1_1_gpio_proxy)        | 
`public  Gpio() = default` | 
`public  Gpio(const `[`Gpio`](#classqemu_1_1_gpio)` & o) = default` | 
`public inline  Gpio(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public void set(bool lvl)` | 
`public inline void set_proxy(std::shared_ptr< `[`GpioProxy`](#classqemu_1_1_gpio_1_1_gpio_proxy)` > proxy)` | 
`public inline void set_event_callback(GpioEventFn cb)` | 

## Members

### `class `[``GpioProxy``](#classqemu_1_1_gpio_1_1_gpio_proxy) {#classqemu_1_1_gpio_1_1_gpio_proxy}




### `public  Gpio() = default` {#classqemu_1_1_gpio_1a74df6a230f5bedac964e53ba8d619b05}





### `public  Gpio(const `[`Gpio`](#classqemu_1_1_gpio)` & o) = default` {#classqemu_1_1_gpio_1aedf51256812cd0f9da588b0ba563cb00}





### `public inline  Gpio(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_gpio_1aacad1d1512f8df3e1708d34502460913}





### `public void set(bool lvl)` {#classqemu_1_1_gpio_1ad4e3013ebe98c3f8af68d7a6d94afa9d}





### `public inline void set_proxy(std::shared_ptr< `[`GpioProxy`](#classqemu_1_1_gpio_1_1_gpio_proxy)` > proxy)` {#classqemu_1_1_gpio_1a9911e503e0945a54c1d3e501c497b3f9}





### `public inline void set_event_callback(GpioEventFn cb)` {#classqemu_1_1_gpio_1a2b7c51a351fa309d90e946a0403df90c}






# class `GpioProxy` {#classqemu_1_1_gpio_1_1_gpio_proxy}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline void event(bool level)` | 
`public inline void set_callback(GpioEventFn cb)` | 
`protected bool m_prev_valid` | 
`protected bool m_prev` | 
`protected GpioEventFn m_cb` | 

## Members

### `public inline void event(bool level)` {#classqemu_1_1_gpio_1_1_gpio_proxy_1a152cab048b1c785cf931040580e55469}





### `public inline void set_callback(GpioEventFn cb)` {#classqemu_1_1_gpio_1_1_gpio_proxy_1a93680dbe920e6932a0613234094aefcc}





### `protected bool m_prev_valid` {#classqemu_1_1_gpio_1_1_gpio_proxy_1a73c0c10f117a9054d7568f230eeea955}





### `protected bool m_prev` {#classqemu_1_1_gpio_1_1_gpio_proxy_1aea42bf748bfce79b00f64b140d811b86}





### `protected GpioEventFn m_cb` {#classqemu_1_1_gpio_1_1_gpio_proxy_1a6f5ad09d5c93cd20aa05e3b6c57d9556}






# class `InvalidLibraryException` {#classqemu_1_1_invalid_library_exception}

```
class InvalidLibraryException
  : public qemu::LibQemuException
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  InvalidLibraryException(const char * lib_name,const char * symbol)` | 
`public inline virtual  ~InvalidLibraryException()` | 

## Members

### `public inline  InvalidLibraryException(const char * lib_name,const char * symbol)` {#classqemu_1_1_invalid_library_exception_1a1f9c247b16cb00fc43bbed65a5e42fd4}





### `public inline virtual  ~InvalidLibraryException()` {#classqemu_1_1_invalid_library_exception_1ac6ee52e61fe5a737dab7b3fc43128ab7}






# class `LibQemu` {#classqemu_1_1_lib_qemu}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  LibQemu(`[`LibraryLoaderIface`](#classqemu_1_1_library_loader_iface)` & library_loader,const char * lib_path)` | 
`public  LibQemu(`[`LibraryLoaderIface`](#classqemu_1_1_library_loader_iface)` & library_loader,Target t)` | 
`public  ~LibQemu()` | 
`public void push_qemu_arg(const char * arg)` | 
`public void push_qemu_arg(std::initializer_list< const char * > args)` | 
`public inline const std::vector< char * > & get_qemu_args() const` | 
`public void init()` | 
`public inline bool is_inited() const` | 
`public void start_gdb_server(std::string port)` | 
`public void lock_iothread()` | 
`public void unlock_iothread()` | 
`public void coroutine_yield()` | 
`public template<class T>`  <br/>`inline T object_new()` | 
`public int64_t get_virtual_clock()` | 
`public `[`Object`](#classqemu_1_1_object)` object_new(const char * type_name)` | 
`public std::shared_ptr< `[`MemoryRegionOps`](#classqemu_1_1_memory_region_ops)` > memory_region_ops_new()` | 
`public `[`Gpio`](#classqemu_1_1_gpio)` gpio_new()` | 
`public std::shared_ptr< `[`Timer`](#classqemu_1_1_timer)` > timer_new()` | 
`public `[`Chardev`](#classqemu_1_1_chardev)` chardev_new(const char * label,const char * type)` | 
`public void tb_invalidate_phys_range(uint64_t start,uint64_t end)` | 

## Members

### `public  LibQemu(`[`LibraryLoaderIface`](#classqemu_1_1_library_loader_iface)` & library_loader,const char * lib_path)` {#classqemu_1_1_lib_qemu_1ad68cbcf4c62434830369a2f37189a590}





### `public  LibQemu(`[`LibraryLoaderIface`](#classqemu_1_1_library_loader_iface)` & library_loader,Target t)` {#classqemu_1_1_lib_qemu_1aecb98c3d8940df3c037891449a317241}





### `public  ~LibQemu()` {#classqemu_1_1_lib_qemu_1a9a52faf2f1449f296cccbd51942ab467}





### `public void push_qemu_arg(const char * arg)` {#classqemu_1_1_lib_qemu_1a5e8d54aaf650faf2b2a572ecefff2ba8}





### `public void push_qemu_arg(std::initializer_list< const char * > args)` {#classqemu_1_1_lib_qemu_1a14dea29732ce6893fe6eedd63a12c7fa}





### `public inline const std::vector< char * > & get_qemu_args() const` {#classqemu_1_1_lib_qemu_1a8b0a2314e81b4abe62a6de954b7d9a5f}





### `public void init()` {#classqemu_1_1_lib_qemu_1aec5375036c59be9c868289eb9e8a67ff}





### `public inline bool is_inited() const` {#classqemu_1_1_lib_qemu_1aeb1b1ccbb05c5c905f8a2e1e85b97287}





### `public void start_gdb_server(std::string port)` {#classqemu_1_1_lib_qemu_1a3497f0ab0188b3b25ffc3fa7af6d44f5}





### `public void lock_iothread()` {#classqemu_1_1_lib_qemu_1aa3012b289dada083c6c6a49449cfba95}





### `public void unlock_iothread()` {#classqemu_1_1_lib_qemu_1a7626e7e503565adc9f16a991ee4ebb38}





### `public void coroutine_yield()` {#classqemu_1_1_lib_qemu_1ae0d852cd874adb6961c02037a80df8d6}





### `public template<class T>`  <br/>`inline T object_new()` {#classqemu_1_1_lib_qemu_1a5aa32afefc467d8907da944eb7773541}





### `public int64_t get_virtual_clock()` {#classqemu_1_1_lib_qemu_1a4b563d01ded272dd6f94a6fe1bfb29d2}





### `public `[`Object`](#classqemu_1_1_object)` object_new(const char * type_name)` {#classqemu_1_1_lib_qemu_1a401e763aeb3076debbe985cf0f16f9aa}





### `public std::shared_ptr< `[`MemoryRegionOps`](#classqemu_1_1_memory_region_ops)` > memory_region_ops_new()` {#classqemu_1_1_lib_qemu_1aa7a1739c4328d2743c5978cdc5b7e473}





### `public `[`Gpio`](#classqemu_1_1_gpio)` gpio_new()` {#classqemu_1_1_lib_qemu_1a6ad204386be7d4a14334da3ac51ce6e4}





### `public std::shared_ptr< `[`Timer`](#classqemu_1_1_timer)` > timer_new()` {#classqemu_1_1_lib_qemu_1a8662a0d9d3d67352b1296d78ec56c8da}





### `public `[`Chardev`](#classqemu_1_1_chardev)` chardev_new(const char * label,const char * type)` {#classqemu_1_1_lib_qemu_1a0e2ab5a0e5bedeab95f18af813221393}





### `public void tb_invalidate_phys_range(uint64_t start,uint64_t end)` {#classqemu_1_1_lib_qemu_1a5c27743873f9f88bf8ed382b23c19868}






# class `LibQemuException` {#classqemu_1_1_lib_qemu_exception}

```
class LibQemuException
  : public std::runtime_error
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  LibQemuException(const char * what)` | 
`public inline  LibQemuException(const std::string & what)` | 
`public inline virtual  ~LibQemuException()` | 

## Members

### `public inline  LibQemuException(const char * what)` {#classqemu_1_1_lib_qemu_exception_1a50db228a6cd33a8131670924f579ef37}





### `public inline  LibQemuException(const std::string & what)` {#classqemu_1_1_lib_qemu_exception_1a3e7a1c40edc92ab7a87d5375babfec2c}





### `public inline virtual  ~LibQemuException()` {#classqemu_1_1_lib_qemu_exception_1a6e6d1ad21333a4242286b17d32b8c5aa}






# class `LibQemuInternals` {#classqemu_1_1_lib_qemu_internals}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  LibQemuInternals(`[`LibQemu`](#classqemu_1_1_lib_qemu)` & inst,LibQemuExports * exports)` | 
`public inline const LibQemuExports & exports() const` | 
`public inline `[`LibQemu`](#classqemu_1_1_lib_qemu)` & get_inst()` | 
`public inline void clear_callbacks(`[`Object`](#classqemu_1_1_object)` obj)` | 
`public inline `[`LibQemuObjectCallback`](#classqemu_1_1_lib_qemu_object_callback)`< Cpu::EndOfLoopCallbackFn > & get_cpu_end_of_loop_cb()` | 
`public inline `[`LibQemuObjectCallback`](#classqemu_1_1_lib_qemu_object_callback)`< Cpu::CpuKickCallbackFn > & get_cpu_kick_cb()` | 
`public inline `[`LibQemuObjectCallback`](#classqemu_1_1_lib_qemu_object_callback)`< CpuRiscv64::MipUpdateCallbackFn > & get_cpu_riscv_mip_update_cb()` | 

## Members

### `public inline  LibQemuInternals(`[`LibQemu`](#classqemu_1_1_lib_qemu)` & inst,LibQemuExports * exports)` {#classqemu_1_1_lib_qemu_internals_1a4c2f26a3c036ca28b6ffa400d1d9f3aa}





### `public inline const LibQemuExports & exports() const` {#classqemu_1_1_lib_qemu_internals_1a5ae73d122a312c5820800cac3782a626}





### `public inline `[`LibQemu`](#classqemu_1_1_lib_qemu)` & get_inst()` {#classqemu_1_1_lib_qemu_internals_1a255fbe8994f517cd7b6047ee1d278a4d}





### `public inline void clear_callbacks(`[`Object`](#classqemu_1_1_object)` obj)` {#classqemu_1_1_lib_qemu_internals_1aaccfba4db89e2f6757cb3d3c050f326e}





### `public inline `[`LibQemuObjectCallback`](#classqemu_1_1_lib_qemu_object_callback)`< Cpu::EndOfLoopCallbackFn > & get_cpu_end_of_loop_cb()` {#classqemu_1_1_lib_qemu_internals_1a98cd32600911e5262b7d0c757fc21935}





### `public inline `[`LibQemuObjectCallback`](#classqemu_1_1_lib_qemu_object_callback)`< Cpu::CpuKickCallbackFn > & get_cpu_kick_cb()` {#classqemu_1_1_lib_qemu_internals_1aecc3b8d23844ceeaeb174282da887592}





### `public inline `[`LibQemuObjectCallback`](#classqemu_1_1_lib_qemu_object_callback)`< CpuRiscv64::MipUpdateCallbackFn > & get_cpu_riscv_mip_update_cb()` {#classqemu_1_1_lib_qemu_internals_1a1f736a95600c4382d18b0b124c6cb1da}






# class `LibQemuObjectCallback` {#classqemu_1_1_lib_qemu_object_callback}

```
class LibQemuObjectCallback
  : public qemu::LibQemuObjectCallbackBase
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline void register_cb(`[`Object`](#classqemu_1_1_object)` obj,T cb)` | 
`public inline virtual void clear(`[`Object`](#classqemu_1_1_object)` obj)` | 
`public template<typename... Args>`  <br/>`inline void call(QemuObject * obj,Args... args) const` | 

## Members

### `public inline void register_cb(`[`Object`](#classqemu_1_1_object)` obj,T cb)` {#classqemu_1_1_lib_qemu_object_callback_1af2b408df63c7550fe7d69bdaec8a1b1a}





### `public inline virtual void clear(`[`Object`](#classqemu_1_1_object)` obj)` {#classqemu_1_1_lib_qemu_object_callback_1a36cb710521b647d0b3404e202820d849}





### `public template<typename... Args>`  <br/>`inline void call(QemuObject * obj,Args... args) const` {#classqemu_1_1_lib_qemu_object_callback_1a9f495b2aa131653c91bc4d1bdf3dae4c}






# class `LibQemuObjectCallbackBase` {#classqemu_1_1_lib_qemu_object_callback_base}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public void clear(`[`Object`](#classqemu_1_1_object)` obj)` | 

## Members

### `public void clear(`[`Object`](#classqemu_1_1_object)` obj)` {#classqemu_1_1_lib_qemu_object_callback_base_1a3227fe95b5af8f26b664d8b9587d6498}






# class `LibraryIface` {#classqemu_1_1_library_iface}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool symbol_exists(const char * symbol)` | 
`public void * get_symbol(const char * symbol)` | 

## Members

### `public bool symbol_exists(const char * symbol)` {#classqemu_1_1_library_iface_1a3d8fd19eef7af0ced28a80877c341d84}





### `public void * get_symbol(const char * symbol)` {#classqemu_1_1_library_iface_1af2bb04dfce9826c1b65665e836782c83}






# class `LibraryLoaderIface` {#classqemu_1_1_library_loader_iface}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public LibraryIfacePtr load_library(const char * lib_name)` | 
`public const char * get_lib_ext()` | 
`public const char * get_last_error()` | 

## Members

### `public LibraryIfacePtr load_library(const char * lib_name)` {#classqemu_1_1_library_loader_iface_1a20a44cc525619626f549cbb3c4560be0}





### `public const char * get_lib_ext()` {#classqemu_1_1_library_loader_iface_1abd131468580d163bc33fcf096ab4ad16}





### `public const char * get_last_error()` {#classqemu_1_1_library_loader_iface_1a3c37e8f8a9d7d2025c2c32b1525c6528}






# class `LibraryLoadErrorException` {#classqemu_1_1_library_load_error_exception}

```
class LibraryLoadErrorException
  : public qemu::LibQemuException
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  LibraryLoadErrorException(const char * lib_name,const char * error)` | 
`public inline virtual  ~LibraryLoadErrorException()` | 

## Members

### `public inline  LibraryLoadErrorException(const char * lib_name,const char * error)` {#classqemu_1_1_library_load_error_exception_1a904579886c70b8dd2d5ea55db9e9b09a}





### `public inline virtual  ~LibraryLoadErrorException()` {#classqemu_1_1_library_load_error_exception_1adedc6edc49c75ca55572a06aeca5a3dd}






# class `MemoryRegion` {#classqemu_1_1_memory_region}

```
class MemoryRegion
  : public qemu::Object
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`MemoryRegion`](#classqemu_1_1_memory_region)` * container` | 
`public  MemoryRegion() = default` | 
`public  MemoryRegion(const `[`MemoryRegion`](#classqemu_1_1_memory_region)` &) = default` | 
`public inline  MemoryRegion(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public  ~MemoryRegion()` | 
`public uint64_t get_size()` | 
`public void init_io(`[`Object`](#classqemu_1_1_object)` owner,const char * name,uint64_t size,MemoryRegionOpsPtr ops)` | 
`public void init_ram_ptr(`[`Object`](#classqemu_1_1_object)` owner,const char * name,uint64_t size,void * ptr)` | 
`public void init_alias(`[`Object`](#classqemu_1_1_object)` owner,const char * name,`[`MemoryRegion`](#classqemu_1_1_memory_region)` & root,uint64_t offset,uint64_t size)` | 
`public void add_subregion(`[`MemoryRegion`](#classqemu_1_1_memory_region)` & mr,uint64_t offset)` | 
`public void del_subregion(const `[`MemoryRegion`](#classqemu_1_1_memory_region)` & mr)` | 
`public MemTxResult dispatch_read(uint64_t addr,uint64_t * data,uint64_t size,`[`MemTxAttrs`](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)` attrs)` | 
`public MemTxResult dispatch_write(uint64_t addr,uint64_t data,uint64_t size,`[`MemTxAttrs`](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)` attrs)` | 
`public inline bool operator<(const `[`MemoryRegion`](#classqemu_1_1_memory_region)` & mr) const` | 

## Members

### `public `[`MemoryRegion`](#classqemu_1_1_memory_region)` * container` {#classqemu_1_1_memory_region_1aa0303918e3dd5f22723ccdc0bdc21676}





### `public  MemoryRegion() = default` {#classqemu_1_1_memory_region_1ac3712cb64d9d21296c0e560c6ecf3ace}





### `public  MemoryRegion(const `[`MemoryRegion`](#classqemu_1_1_memory_region)` &) = default` {#classqemu_1_1_memory_region_1a346e535abb59858b39a421b81cc7e494}





### `public inline  MemoryRegion(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_memory_region_1a44a064f36b0ed82e1295ce503b957d24}





### `public  ~MemoryRegion()` {#classqemu_1_1_memory_region_1af795c4b4071da5106476fd04845917c5}





### `public uint64_t get_size()` {#classqemu_1_1_memory_region_1a8fe8b345bd7f50d324f32253c61cc580}





### `public void init_io(`[`Object`](#classqemu_1_1_object)` owner,const char * name,uint64_t size,MemoryRegionOpsPtr ops)` {#classqemu_1_1_memory_region_1ae2841bed11d50d030b0babef9c731f37}





### `public void init_ram_ptr(`[`Object`](#classqemu_1_1_object)` owner,const char * name,uint64_t size,void * ptr)` {#classqemu_1_1_memory_region_1abfe8d1b8beb9bf900934cfd09e0370a4}





### `public void init_alias(`[`Object`](#classqemu_1_1_object)` owner,const char * name,`[`MemoryRegion`](#classqemu_1_1_memory_region)` & root,uint64_t offset,uint64_t size)` {#classqemu_1_1_memory_region_1ad31104788b6f9c6cce167eeb4ec734e7}





### `public void add_subregion(`[`MemoryRegion`](#classqemu_1_1_memory_region)` & mr,uint64_t offset)` {#classqemu_1_1_memory_region_1ae77e0ac04ad9b1dcc6039c2ff3177d55}





### `public void del_subregion(const `[`MemoryRegion`](#classqemu_1_1_memory_region)` & mr)` {#classqemu_1_1_memory_region_1a8ebd479f1cffe2245e4c1d25d4b2fb41}





### `public MemTxResult dispatch_read(uint64_t addr,uint64_t * data,uint64_t size,`[`MemTxAttrs`](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)` attrs)` {#classqemu_1_1_memory_region_1abe4dc7b08ad39d761f5930213b29e97d}





### `public MemTxResult dispatch_write(uint64_t addr,uint64_t data,uint64_t size,`[`MemTxAttrs`](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)` attrs)` {#classqemu_1_1_memory_region_1abae5226e10204ae71d0b47fe85439b2a}





### `public inline bool operator<(const `[`MemoryRegion`](#classqemu_1_1_memory_region)` & mr) const` {#classqemu_1_1_memory_region_1a868affa25b4f89d804553945f8051edd}






# class `MemoryRegionOps` {#classqemu_1_1_memory_region_ops}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`struct `[``MemTxAttrs``](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)        | 
`public  MemoryRegionOps(QemuMemoryRegionOps * ops,std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > internals)` | 
`public  ~MemoryRegionOps()` | 
`public void set_read_callback(ReadCallback cb)` | 
`public void set_write_callback(WriteCallback cb)` | 
`public void set_max_access_size(unsigned size)` | 
`public inline ReadCallback get_read_callback()` | 
`public inline WriteCallback get_write_callback()` | 
`public inline QemuMemoryRegionOps * get_qemu_mr_ops()` | 

## Members

### `struct `[``MemTxAttrs``](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs) {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs}




### `public  MemoryRegionOps(QemuMemoryRegionOps * ops,std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > internals)` {#classqemu_1_1_memory_region_ops_1abbaa62e79600f92c2fa0876b366de605}





### `public  ~MemoryRegionOps()` {#classqemu_1_1_memory_region_ops_1a68bf2e511974947f1519b01e70c5c34f}





### `public void set_read_callback(ReadCallback cb)` {#classqemu_1_1_memory_region_ops_1aacf3916e2a3188a41dbc10ec5edde059}





### `public void set_write_callback(WriteCallback cb)` {#classqemu_1_1_memory_region_ops_1af1f4b3dde8d89a73bf80c59878cb4762}





### `public void set_max_access_size(unsigned size)` {#classqemu_1_1_memory_region_ops_1a96747c118e4223441d16f1a433b7f9e3}





### `public inline ReadCallback get_read_callback()` {#classqemu_1_1_memory_region_ops_1a52cf9edf782553291a47c9d1cd27c626}





### `public inline WriteCallback get_write_callback()` {#classqemu_1_1_memory_region_ops_1a8c4bc9132fa951ebdde3fd68959cd314}





### `public inline QemuMemoryRegionOps * get_qemu_mr_ops()` {#classqemu_1_1_memory_region_ops_1a0b066cb4aca70999d8332a9f4a63b3f8}






# struct `MemTxAttrs` {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool secure` | 
`public bool exclusive` | 
`public bool debug` | 
`public  MemTxAttrs() = default` | 
`public  MemTxAttrs(const ::`[`MemTxAttrs`](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)` & qemu_attrs)` | 

## Members

### `public bool secure` {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs_1a30e307997d52cf32053bacc1825e60e4}





### `public bool exclusive` {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs_1a6ed503a1851f9ed7d166aaf9e9cb63f7}





### `public bool debug` {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs_1afebe6ec75b899ce48fe14bcf6773e909}





### `public  MemTxAttrs() = default` {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs_1acd9fd22909e0c5c1175d54ab49277bf9}





### `public  MemTxAttrs(const ::`[`MemTxAttrs`](#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs)` & qemu_attrs)` {#structqemu_1_1_memory_region_ops_1_1_mem_tx_attrs_1a2905c308ef16bdfa0da0981c8666bd72}






# class `Object` {#classqemu_1_1_object}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Object() = default` | 
`public  Object(QemuObject * obj,std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > & internals)` | 
`public  Object(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public  Object(`[`Object`](#classqemu_1_1_object)` && o)` | 
`public `[`Object`](#classqemu_1_1_object)` & operator=(`[`Object`](#classqemu_1_1_object)` o)` | 
`public virtual  ~Object()` | 
`public inline bool valid() const` | 
`public void set_prop_bool(const char * name,bool val)` | 
`public void set_prop_int(const char * name,int64_t val)` | 
`public void set_prop_str(const char * name,const char * val)` | 
`public void set_prop_link(const char * name,const `[`Object`](#classqemu_1_1_object)` & link)` | 
`public inline QemuObject * get_qemu_obj()` | 
`public `[`LibQemu`](#classqemu_1_1_lib_qemu)` & get_inst()` | 
`public inline uintptr_t get_inst_id() const` | 
`public inline bool same_inst_as(const `[`Object`](#classqemu_1_1_object)` & o) const` | 
`public template<class T>`  <br/>`inline bool check_cast() const` | 
`public void clear_callbacks()` | 
`protected QemuObject * m_obj` | 
`protected std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > m_int` | 
`protected inline bool check_cast_by_type(const char * type_name) const` | 

## Members

### `public  Object() = default` {#classqemu_1_1_object_1ac677fb3d16bbc65b786ce438789c0416}





### `public  Object(QemuObject * obj,std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > & internals)` {#classqemu_1_1_object_1a0205a75b78aeeaeeecfec729b6016d42}





### `public  Object(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_object_1a73d4084e3a080f4691182c58328bd166}





### `public  Object(`[`Object`](#classqemu_1_1_object)` && o)` {#classqemu_1_1_object_1afa3be8d52ab42782e45a267acae16ebd}





### `public `[`Object`](#classqemu_1_1_object)` & operator=(`[`Object`](#classqemu_1_1_object)` o)` {#classqemu_1_1_object_1abe84ca84fab2d19197a19fc6e1d6818f}





### `public virtual  ~Object()` {#classqemu_1_1_object_1ad1ee761cd19b80238104a19c3f272ded}





### `public inline bool valid() const` {#classqemu_1_1_object_1afed57dc9c5188cd8b1f7d34e8d11925f}





### `public void set_prop_bool(const char * name,bool val)` {#classqemu_1_1_object_1aca20489d18c9e687fc0be6198ea946a2}





### `public void set_prop_int(const char * name,int64_t val)` {#classqemu_1_1_object_1a61840ada74f3eab64daefa94f25a83e5}





### `public void set_prop_str(const char * name,const char * val)` {#classqemu_1_1_object_1a66424902534f94f415214d77cb837383}





### `public void set_prop_link(const char * name,const `[`Object`](#classqemu_1_1_object)` & link)` {#classqemu_1_1_object_1a28caf03f78ddaf7b0d627a8ac3652d09}





### `public inline QemuObject * get_qemu_obj()` {#classqemu_1_1_object_1afc0e5dda7894bd0e999e1a2bb639fb00}





### `public `[`LibQemu`](#classqemu_1_1_lib_qemu)` & get_inst()` {#classqemu_1_1_object_1af397e002ec6169fb97bfef0d4197b4e0}





### `public inline uintptr_t get_inst_id() const` {#classqemu_1_1_object_1a9b639c19ca10e517fea9ce0fec31d522}





### `public inline bool same_inst_as(const `[`Object`](#classqemu_1_1_object)` & o) const` {#classqemu_1_1_object_1a24a4ff19bbfeb3ebcf85f06467b78bbc}





### `public template<class T>`  <br/>`inline bool check_cast() const` {#classqemu_1_1_object_1ac605f492e8984ceff7ea7acc410ff41f}





### `public void clear_callbacks()` {#classqemu_1_1_object_1ad0e0d24683873d72086aeaf3f8a61f7c}





### `protected QemuObject * m_obj` {#classqemu_1_1_object_1a90689e8f9f0b7ca0910080d6b3e6a7ae}





### `protected std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > m_int` {#classqemu_1_1_object_1a0a325d3f647c1a30a3e2dba842e69e57}





### `protected inline bool check_cast_by_type(const char * type_name) const` {#classqemu_1_1_object_1aa421d110b568981e673c8b34b55816f5}






# class `SetPropertyException` {#classqemu_1_1_set_property_exception}

```
class SetPropertyException
  : public qemu::LibQemuException
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  SetPropertyException(const char * type,const char * name)` | 
`public inline virtual  ~SetPropertyException()` | 

## Members

### `public inline  SetPropertyException(const char * type,const char * name)` {#classqemu_1_1_set_property_exception_1ac07917d9b097f8711528f646778c5a56}





### `public inline virtual  ~SetPropertyException()` {#classqemu_1_1_set_property_exception_1ae21991fbf2cc67fcb7b54820b26abbfd}






# class `SysBusDevice` {#classqemu_1_1_sys_bus_device}

```
class SysBusDevice
  : public qemu::Device
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  SysBusDevice() = default` | 
`public  SysBusDevice(const `[`SysBusDevice`](#classqemu_1_1_sys_bus_device)` &) = default` | 
`public inline  SysBusDevice(const `[`Object`](#classqemu_1_1_object)` & o)` | 
`public `[`MemoryRegion`](#classqemu_1_1_memory_region)` mmio_get_region(int id)` | 
`public void connect_gpio_out(int idx,`[`Gpio`](#classqemu_1_1_gpio)` gpio)` | 

## Members

### `public  SysBusDevice() = default` {#classqemu_1_1_sys_bus_device_1a3954126ada6f53e3f80e7d9392dd67f8}





### `public  SysBusDevice(const `[`SysBusDevice`](#classqemu_1_1_sys_bus_device)` &) = default` {#classqemu_1_1_sys_bus_device_1ae3855add483129a1d97fa862fa7a10d5}





### `public inline  SysBusDevice(const `[`Object`](#classqemu_1_1_object)` & o)` {#classqemu_1_1_sys_bus_device_1a221a1bfea569df85754481e623a07b71}





### `public `[`MemoryRegion`](#classqemu_1_1_memory_region)` mmio_get_region(int id)` {#classqemu_1_1_sys_bus_device_1a5c6efdf31f7cee45368449b13d0abbe2}





### `public void connect_gpio_out(int idx,`[`Gpio`](#classqemu_1_1_gpio)` gpio)` {#classqemu_1_1_sys_bus_device_1ac93954cecb3a5050ba0e032ed264a8be}






# class `TargetNotSupportedException` {#classqemu_1_1_target_not_supported_exception}

```
class TargetNotSupportedException
  : public qemu::LibQemuException
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  TargetNotSupportedException(Target t)` | 
`public inline virtual  ~TargetNotSupportedException()` | 

## Members

### `public inline  TargetNotSupportedException(Target t)` {#classqemu_1_1_target_not_supported_exception_1a373b5e795697c7c7508221ea8df9f015}





### `public inline virtual  ~TargetNotSupportedException()` {#classqemu_1_1_target_not_supported_exception_1a45372ea9a8f803f1e006b3835329e1a9}






# class `Timer` {#classqemu_1_1_timer}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  Timer(std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > internals)` | 
`public  ~Timer()` | 
`public void set_callback(TimerCallbackFn cb)` | 
`public void mod(int64_t deadline)` | 
`public void del()` | 

## Members

### `public  Timer(std::shared_ptr< `[`LibQemuInternals`](#classqemu_1_1_lib_qemu_internals)` > internals)` {#classqemu_1_1_timer_1a7979d357348d786ede1af2419ecb878c}





### `public  ~Timer()` {#classqemu_1_1_timer_1a16e2f51ea652952426fe4d63f1b3a1a2}





### `public void set_callback(TimerCallbackFn cb)` {#classqemu_1_1_timer_1a349014a135ec9a19b17160fba7e96fa9}





### `public void mod(int64_t deadline)` {#classqemu_1_1_timer_1a004d685d997f42be1e5335cb9ad7135c}





### `public void del()` {#classqemu_1_1_timer_1af1d060cea3990931d2b72e99c316d260}






# namespace `std`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# class `DefaultLibraryLoader` {#class_default_library_loader}

```
class DefaultLibraryLoader
  : public qemu::LibraryLoaderIface
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline virtual qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char * lib_name)` | 
`public inline virtual const char * get_lib_ext()` | 
`public inline virtual const char * get_last_error()` | 

## Members

### `public inline virtual qemu::LibraryLoaderIface::LibraryIfacePtr load_library(const char * lib_name)` {#class_default_library_loader_1ac68a378ce9ba49d55d2ad3be3ff176a7}





### `public inline virtual const char * get_lib_ext()` {#class_default_library_loader_1ab2f40815d0ce39e97537f18e02a8d39b}





### `public inline virtual const char * get_last_error()` {#class_default_library_loader_1a4c4f9bc6dd923329d47ac69d5d59f750}






# class `Library` {#class_library}

```
class Library
  : public qemu::LibraryIface
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  Library(void * lib)` | 
`public inline virtual bool symbol_exists(const char * name)` | 
`public inline virtual void * get_symbol(const char * name)` | 

## Members

### `public inline  Library(void * lib)` {#class_library_1ad31bdf8efd5d4417f90c98936f3da994}





### `public inline virtual bool symbol_exists(const char * name)` {#class_library_1a5eac9b6ae6f888927eef75bdbf353a04}





### `public inline virtual void * get_symbol(const char * name)` {#class_library_1af9f5342966e2db20dc13be6ada7f6792}






