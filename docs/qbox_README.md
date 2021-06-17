# namespace `qemu`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# namespace `sc_core`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# namespace `std`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# namespace `tlm`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# class `QemuInstanceDmiManager` {#class_qemu_instance_dmi_manager}


Handles the DMI regions at the QEMU instance level.

This class handles the DMI regions at the level of a QEMU instance. For a given DMI region, we need to use a unique memory region (called the global memory region, in a sense that it is global to all the CPUs in the instance). Each CPU is then supposed to create an alias to this region to be able to access it. This is required to ensure QEMU sees this region as a unique piece of memory. Creating multiple regions mapping to the same host address leads QEMU into thinking that those are different data, and it won't properly invalidate corresponding TBs if CPUs do SMC (self modifying code).

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[``QemuContainer``](#class_qemu_instance_dmi_manager_1_1_qemu_container)        | 
`struct `[``DmiInfo``](#struct_qemu_instance_dmi_manager_1_1_dmi_info)        | 
`public inline  QemuInstanceDmiManager(qemu::LibQemu & inst)` | 
`public  QemuInstanceDmiManager(const `[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` &) = delete` | 
`public inline  QemuInstanceDmiManager(`[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` && a)` | 
`public inline void init()` | 
`public inline void get_global_mr(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` | Fill DMI info with the corresponding global memory region.
`protected qemu::LibQemu & m_inst` | 
`protected `[`QemuContainer`](#class_qemu_instance_dmi_manager_1_1_qemu_container)` m_mr_container` | 
`protected std::mutex m_mutex` | 
`protected std::map< DmiInfo::Key, `[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` > m_dmis` | 
`protected inline void create_global_mr(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` | 

## Members

### `class `[``QemuContainer``](#class_qemu_instance_dmi_manager_1_1_qemu_container) {#class_qemu_instance_dmi_manager_1_1_qemu_container}




### `struct `[``DmiInfo``](#struct_qemu_instance_dmi_manager_1_1_dmi_info) {#struct_qemu_instance_dmi_manager_1_1_dmi_info}




### `public inline  QemuInstanceDmiManager(qemu::LibQemu & inst)` {#class_qemu_instance_dmi_manager_1abb2fec6da476baac7bb7cc4d6fdd7998}





### `public  QemuInstanceDmiManager(const `[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` &) = delete` {#class_qemu_instance_dmi_manager_1a6675db2c26e2b5ef2dcc2aa36552e9c6}





### `public inline  QemuInstanceDmiManager(`[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` && a)` {#class_qemu_instance_dmi_manager_1adbac3c00001a02f426e4ca6abae89891}





### `public inline void init()` {#class_qemu_instance_dmi_manager_1ad3dc1849eb47333c6d4073ce0ad71b98}





### `public inline void get_global_mr(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` {#class_qemu_instance_dmi_manager_1aed894ad4129c7e6a54c324f7046bfc7a}

Fill DMI info with the corresponding global memory region.

Fill the DMI info info with the global memory region matching the rest of info. If the global MR does not exist yet, it is created. Otherwise the already existing one is returned.


#### Parameters
* `info` The DMI info to use and to fill with the global MR

### `protected qemu::LibQemu & m_inst` {#class_qemu_instance_dmi_manager_1a0ffee4cd03b9ecc8af5323ed2ada564e}





### `protected `[`QemuContainer`](#class_qemu_instance_dmi_manager_1_1_qemu_container)` m_mr_container` {#class_qemu_instance_dmi_manager_1aa34ce116000da1aaa412b3dba5d638ef}





### `protected std::mutex m_mutex` {#class_qemu_instance_dmi_manager_1a8bd09ef69d5b761a53fe54807f554910}





### `protected std::map< DmiInfo::Key, `[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` > m_dmis` {#class_qemu_instance_dmi_manager_1a1878a9d145744e4028c1614edd58d0ce}





### `protected inline void create_global_mr(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` {#class_qemu_instance_dmi_manager_1ac20b2bf8dabcdb92875e9a8e287b57e5}






# class `QemuContainer` {#class_qemu_instance_dmi_manager_1_1_qemu_container}

```
class QemuContainer
  : public qemu::Object
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  QemuContainer() = default` | 
`public  QemuContainer(const `[`QemuContainer`](#class_qemu_instance_dmi_manager_1_1_qemu_container)` & o) = default` | 
`public inline  QemuContainer(const Object & o)` | 

## Members

### `public  QemuContainer() = default` {#class_qemu_instance_dmi_manager_1_1_qemu_container_1ae0d0d959be59c131585897e289e05d4d}





### `public  QemuContainer(const `[`QemuContainer`](#class_qemu_instance_dmi_manager_1_1_qemu_container)` & o) = default` {#class_qemu_instance_dmi_manager_1_1_qemu_container_1aea7b726c366184f17ffe18f77f77fa7b}





### `public inline  QemuContainer(const Object & o)` {#class_qemu_instance_dmi_manager_1_1_qemu_container_1a49f2128b809f118a9db862011e6d60ca}






# struct `DmiInfo` {#struct_qemu_instance_dmi_manager_1_1_dmi_info}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint64_t start` | 
`public uint64_t end` | 
`public void * ptr` | 
`public qemu::MemoryRegion mr` | 
`public  DmiInfo() = default` | 
`public inline  explicit DmiInfo(const tlm::tlm_dmi & info)` | 
`public inline uint64_t get_size() const` | 
`public inline Key get_key() const` | 

## Members

### `public uint64_t start` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1af730fa33c2a16ecf1c162f8a47a927df}





### `public uint64_t end` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1a6ab656148bd1a747b72624623c09738a}





### `public void * ptr` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1a3c0aa485ad517a3db9f799d85b4d9dff}





### `public qemu::MemoryRegion mr` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1a85fd8a4de8ab54d98269f45067ed973f}





### `public  DmiInfo() = default` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1a0ecf41577e125ad91d3c85c6f8de4b27}





### `public inline  explicit DmiInfo(const tlm::tlm_dmi & info)` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1a970c36fd554955429ee4ed07f4af46d2}





### `public inline uint64_t get_size() const` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1a58efabefe50ac2f4aa5dfafd7ddff89c}





### `public inline Key get_key() const` {#struct_qemu_instance_dmi_manager_1_1_dmi_info_1aa8bae21fc045ed3061b950f8f29b961e}






# class `LockedQemuInstanceDmiManager` {#class_locked_qemu_instance_dmi_manager}


A locked [QemuInstanceDmiManager](#class_qemu_instance_dmi_manager).

This class is a wrapper around [QemuInstanceDmiManager](#class_qemu_instance_dmi_manager) that ensure safe accesses to it. As long as an instance of this class is live, the underlying [QemuInstanceDmiManager](#class_qemu_instance_dmi_manager) is locked. It gets unlocked once the object goes out of scope.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  LockedQemuInstanceDmiManager(`[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` & inst)` | 
`public  LockedQemuInstanceDmiManager(const `[`LockedQemuInstanceDmiManager`](#class_locked_qemu_instance_dmi_manager)` &) = delete` | 
`public  LockedQemuInstanceDmiManager(`[`LockedQemuInstanceDmiManager`](#class_locked_qemu_instance_dmi_manager)` &&) = default` | 
`public inline void get_global_mr(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` | 
`protected `[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` & m_inst` | 
`protected std::unique_lock< std::mutex > m_lock` | 

## Members

### `public inline  LockedQemuInstanceDmiManager(`[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` & inst)` {#class_locked_qemu_instance_dmi_manager_1ad3e24e8ddf47933c7eff8d46303aaab8}





### `public  LockedQemuInstanceDmiManager(const `[`LockedQemuInstanceDmiManager`](#class_locked_qemu_instance_dmi_manager)` &) = delete` {#class_locked_qemu_instance_dmi_manager_1a5a8b2478671c0e508eacdf6f5659dab2}





### `public  LockedQemuInstanceDmiManager(`[`LockedQemuInstanceDmiManager`](#class_locked_qemu_instance_dmi_manager)` &&) = default` {#class_locked_qemu_instance_dmi_manager_1aaa2d5a59efb5399eca4b06ac2bd746b0}





### `public inline void get_global_mr(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` {#class_locked_qemu_instance_dmi_manager_1aad9f1110164a86b374791279ba2f4b57}



[QemuInstanceDmiManager::get_global_mr](#class_qemu_instance_dmi_manager_1aed894ad4129c7e6a54c324f7046bfc7a)

### `protected `[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` & m_inst` {#class_locked_qemu_instance_dmi_manager_1a434bb9255ec815f6f581e6d4c34f5ed8}





### `protected std::unique_lock< std::mutex > m_lock` {#class_locked_qemu_instance_dmi_manager_1ad11ea62c7146c876d4ba0d23d3b2cfa6}






# class `QboxException` {#class_qbox_exception}

```
class QboxException
  : public std::runtime_error
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QboxException(const char * what)` | 
`public inline virtual  ~QboxException()` | 

## Members

### `public inline  QboxException(const char * what)` {#class_qbox_exception_1ad22db7dc3cfbd1d2967d11e7bb18e788}





### `public inline virtual  ~QboxException()` {#class_qbox_exception_1aabde4957b8b4a25f73f0beac89a075c8}






# class `QemuCpu` {#class_qemu_cpu}

```
class QemuCpu
  : public QemuDevice
  : public QemuInitiatorIface
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public cci::cci_param< bool > p_icount` | 
`public cci::cci_param< int > p_icount_mips` | 
`public cci::cci_param< unsigned int > p_gdb_port` | 
`public cci::cci_param< std::string > p_sync_policy` | 
`public `[`QemuInitiatorSocket`](#class_qemu_initiator_socket)` socket` | 
`public  SC_HAS_PROCESS(`[`QemuCpu`](#class_qemu_cpu)`)` | 
`public inline  QemuCpu(const sc_core::sc_module_name & name,`[`QemuInstance`](#class_qemu_instance)` & inst,const std::string & type_name)` | 
`public inline virtual  ~QemuCpu()` | 
`public inline virtual void before_end_of_elaboration()` | 
`public inline virtual void end_of_elaboration()` | 
`public inline virtual void start_of_simulation()` | 
`public inline virtual void initiator_customize_tlm_payload(TlmPayload & payload)` | 
`public inline virtual sc_core::sc_time initiator_get_local_time()` | 
`public inline virtual void initiator_set_local_time(const sc_core::sc_time & t)` | 
`protected gs::RunOnSysC m_on_sysc` | 
`protected std::shared_ptr< qemu::Timer > m_deadline_timer` | 
`protected bool m_coroutines` | 
`protected qemu::Cpu m_cpu` | 
`protected gs::async_event m_qemu_kick_ev` | 
`protected sc_core::sc_event_or_list m_external_ev` | 
`protected int64_t m_last_vclock` | 
`protected std::shared_ptr< gs::tlm_quantumkeeper_extended > m_qk` | 
`protected inline void create_quantum_keeper()` | 
`protected inline void set_qemu_instance_options()` | 
`protected inline void deadline_timer_cb()` | 
`protected inline void wait_for_work()` | 
`protected inline void rearm_deadline_timer()` | 
`protected inline void prepare_run_cpu()` | 
`protected inline void run_cpu_loop()` | 
`protected inline void sync_with_kernel()` | 
`protected inline void kick_cb()` | 
`protected inline void end_of_loop_cb()` | 
`protected inline void mainloop_thread_coroutine()` | 

## Members

### `public cci::cci_param< bool > p_icount` {#class_qemu_cpu_1a571c999efb6d2cf4ed6e9e1f0acc03e7}





### `public cci::cci_param< int > p_icount_mips` {#class_qemu_cpu_1ab90406a5649d56237039c7117926b318}





### `public cci::cci_param< unsigned int > p_gdb_port` {#class_qemu_cpu_1aba10f9871ce1f00745a57b719052e60a}





### `public cci::cci_param< std::string > p_sync_policy` {#class_qemu_cpu_1a6739941a8b7e75f74a2aa0fa5aa4f6e8}





### `public `[`QemuInitiatorSocket`](#class_qemu_initiator_socket)` socket` {#class_qemu_cpu_1a13521a1d7f736249be67c018fdcd632a}





### `public  SC_HAS_PROCESS(`[`QemuCpu`](#class_qemu_cpu)`)` {#class_qemu_cpu_1a16acd31d80280eb63a54cbdadd59d14c}





### `public inline  QemuCpu(const sc_core::sc_module_name & name,`[`QemuInstance`](#class_qemu_instance)` & inst,const std::string & type_name)` {#class_qemu_cpu_1a56ca567fe968f8017853074afaab329f}





### `public inline virtual  ~QemuCpu()` {#class_qemu_cpu_1ad8d8ce34c805055e915fea04d3d3b652}





### `public inline virtual void before_end_of_elaboration()` {#class_qemu_cpu_1a16de26ded57855ab604b1ad7043f2d84}





### `public inline virtual void end_of_elaboration()` {#class_qemu_cpu_1ae2bd90e1763a03b9c3d4164e9f8e6bc6}





### `public inline virtual void start_of_simulation()` {#class_qemu_cpu_1af73547489f05f6db710d7cf9259eea79}





### `public inline virtual void initiator_customize_tlm_payload(TlmPayload & payload)` {#class_qemu_cpu_1ab946fb1784cc03f28ddc9b9c82157354}





### `public inline virtual sc_core::sc_time initiator_get_local_time()` {#class_qemu_cpu_1a94d9ec985a7b1fe621950869f07be140}





### `public inline virtual void initiator_set_local_time(const sc_core::sc_time & t)` {#class_qemu_cpu_1a0908feb5a42c24a8c50cf2ba388c151f}





### `protected gs::RunOnSysC m_on_sysc` {#class_qemu_cpu_1a24df1b914fdbdfe337917429e1ebfa14}





### `protected std::shared_ptr< qemu::Timer > m_deadline_timer` {#class_qemu_cpu_1a9ed85256d31692176a58f53737241d65}





### `protected bool m_coroutines` {#class_qemu_cpu_1ab9c21e1cf3f933f687e9c0538ab2b6f8}





### `protected qemu::Cpu m_cpu` {#class_qemu_cpu_1a68ed30eb41b2fe2ad425bda08e7f3d2d}





### `protected gs::async_event m_qemu_kick_ev` {#class_qemu_cpu_1a1ab17ef33592d47bdc03b3691c63972e}





### `protected sc_core::sc_event_or_list m_external_ev` {#class_qemu_cpu_1a478e9951e916a968db6b778b9ca78c97}





### `protected int64_t m_last_vclock` {#class_qemu_cpu_1ace489efb8aff322dd7a8edc83c64a493}





### `protected std::shared_ptr< gs::tlm_quantumkeeper_extended > m_qk` {#class_qemu_cpu_1a5fbaef0bb62914f8bbe6461c17945d31}





### `protected inline void create_quantum_keeper()` {#class_qemu_cpu_1a09b9052807192ac18217515e3a9f35ac}





### `protected inline void set_qemu_instance_options()` {#class_qemu_cpu_1aa6d4d8793116730bc332b623933834a9}





### `protected inline void deadline_timer_cb()` {#class_qemu_cpu_1a74163baf42cb77e24a1d15010fe39bfc}





### `protected inline void wait_for_work()` {#class_qemu_cpu_1a59f8c4d6e7c97fba6407f4b0c11a2ed9}





### `protected inline void rearm_deadline_timer()` {#class_qemu_cpu_1a87bb923537df4ca2194e868374527b9d}





### `protected inline void prepare_run_cpu()` {#class_qemu_cpu_1a73c4747fd240de1d4071dc6c79d42a61}





### `protected inline void run_cpu_loop()` {#class_qemu_cpu_1a2691fcdd144902af12110471f9dde3d2}





### `protected inline void sync_with_kernel()` {#class_qemu_cpu_1add9bdbb9658902bc9705891f82e7215b}





### `protected inline void kick_cb()` {#class_qemu_cpu_1a29fcc8e94954673ab0788d6bfb3e7096}





### `protected inline void end_of_loop_cb()` {#class_qemu_cpu_1a33681086ad80bfade5563b0ac6dd1cfe}





### `protected inline void mainloop_thread_coroutine()` {#class_qemu_cpu_1a0571a39ba60359fef9a5266a37ac3e44}






# class `QemuCpuHintTlmExtension` {#class_qemu_cpu_hint_tlm_extension}

```
class QemuCpuHintTlmExtension
  : public tlm::tlm_extension< QemuCpuHintTlmExtension >
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  QemuCpuHintTlmExtension() = default` | 
`public  QemuCpuHintTlmExtension(const `[`QemuCpuHintTlmExtension`](#class_qemu_cpu_hint_tlm_extension)` &) = default` | 
`public inline  QemuCpuHintTlmExtension(qemu::Cpu cpu)` | 
`public inline virtual tlm_extension_base * clone() const` | 
`public inline virtual void copy_from(tlm_extension_base const & ext)` | 
`public inline qemu::Cpu get_cpu() const` | 

## Members

### `public  QemuCpuHintTlmExtension() = default` {#class_qemu_cpu_hint_tlm_extension_1a5bd0eb7cbfbd2355225a865364175c52}





### `public  QemuCpuHintTlmExtension(const `[`QemuCpuHintTlmExtension`](#class_qemu_cpu_hint_tlm_extension)` &) = default` {#class_qemu_cpu_hint_tlm_extension_1a35602c57b4302d7bd5c67e2180b9982b}





### `public inline  QemuCpuHintTlmExtension(qemu::Cpu cpu)` {#class_qemu_cpu_hint_tlm_extension_1a02b8cb462db68f2b5ae550dcc90ad74c}





### `public inline virtual tlm_extension_base * clone() const` {#class_qemu_cpu_hint_tlm_extension_1ac637b39576cc64d023d7cce569372ce2}





### `public inline virtual void copy_from(tlm_extension_base const & ext)` {#class_qemu_cpu_hint_tlm_extension_1a45b766788e03d0dffe0bd72b4839c1d4}





### `public inline qemu::Cpu get_cpu() const` {#class_qemu_cpu_hint_tlm_extension_1a9950bdacf28e447cd0e2dbe1a24f92f9}






# class `QemuCpuRiscv64` {#class_qemu_cpu_riscv64}

```
class QemuCpuRiscv64
  : public QemuCpu
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuCpuRiscv64(const sc_core::sc_module_name & name,`[`QemuInstance`](#class_qemu_instance)` & inst,const char * model,uint64_t hartid)` | 
`public inline virtual void before_end_of_elaboration()` | 
`protected uint64_t m_hartid` | 
`protected gs::async_event m_irq_ev` | 
`protected inline void mip_update_cb(uint32_t value)` | 

## Members

### `public inline  QemuCpuRiscv64(const sc_core::sc_module_name & name,`[`QemuInstance`](#class_qemu_instance)` & inst,const char * model,uint64_t hartid)` {#class_qemu_cpu_riscv64_1a6711e73e3b7ac70547bb7abda10c6997}





### `public inline virtual void before_end_of_elaboration()` {#class_qemu_cpu_riscv64_1a1be4ef74a9a285222d7eb7c62302d220}





### `protected uint64_t m_hartid` {#class_qemu_cpu_riscv64_1a64e2e0ff287af81677575718f739cdc0}





### `protected gs::async_event m_irq_ev` {#class_qemu_cpu_riscv64_1a14a0e09c8f6ff44c5153b337710f8912}





### `protected inline void mip_update_cb(uint32_t value)` {#class_qemu_cpu_riscv64_1afaeefb32a8e37a938be88a773f973aa1}






# class `QemuCpuRiscv64Rv64` {#class_qemu_cpu_riscv64_rv64}

```
class QemuCpuRiscv64Rv64
  : public QemuCpuRiscv64
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuCpuRiscv64Rv64(const sc_core::sc_module_name & n,`[`QemuInstance`](#class_qemu_instance)` & inst,uint64_t hartid)` | 

## Members

### `public inline  QemuCpuRiscv64Rv64(const sc_core::sc_module_name & n,`[`QemuInstance`](#class_qemu_instance)` & inst,uint64_t hartid)` {#class_qemu_cpu_riscv64_rv64_1a49332928df0a6fa9495e2f8eb606b6c6}






# class `QemuDevice` {#class_qemu_device}

```
class QemuDevice
  : public sc_core::sc_module
```  

QEMU device abstraction as a SystemC module.

This class abstract a QEMU device as a SystemC module. It is constructed using the QEMU instance it will lie in, and the QOM type name corresponding to the device. This class is meant to be inherited from by children classes that implement a given device.

The elaboration flow is as follows:

* At construct time, nothing happen on the QEMU side.


* When before_end_of_elaboration is called, the QEMU object correponding to this component is created. Children classes should always call the parent method when overriding it. Usually, they start by calling it and then set the QEMU properties on the device.


* When end_of_elaboration is called, the device is realized. No more property can be set (unless particular cases such as some link properties) and the device can now be connected to busses and GPIO.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuDevice(const sc_core::sc_module_name & name,`[`QemuInstance`](#class_qemu_instance)` & inst,const char * qom_type)` | Construct a QEMU device.
`public inline virtual  ~QemuDevice()` | 
`public inline virtual void before_end_of_elaboration()` | 
`public inline virtual void end_of_elaboration()` | 
`public inline const char * get_qom_type() const` | 
`public inline qemu::Device get_qemu_dev()` | 
`public inline `[`QemuInstance`](#class_qemu_instance)` & get_qemu_inst()` | 
`protected `[`QemuInstance`](#class_qemu_instance)` & m_inst` | 
`protected qemu::Device m_dev` | 
`protected bool m_realized` | 
`protected inline void realize()` | 

## Members

### `public inline  QemuDevice(const sc_core::sc_module_name & name,`[`QemuInstance`](#class_qemu_instance)` & inst,const char * qom_type)` {#class_qemu_device_1acb3e2b0907fd4d651bff0fef44dc6ce3}

Construct a QEMU device.

#### Parameters
* `name` SystemC module name 


* `inst` QEMU instance the device will be created in 


* `qom_type` Device QOM type name

### `public inline virtual  ~QemuDevice()` {#class_qemu_device_1a11178f36edff1fd1524faa6fb5b07c93}





### `public inline virtual void before_end_of_elaboration()` {#class_qemu_device_1ac63346917ef48c92f5b1b398ff143b57}





### `public inline virtual void end_of_elaboration()` {#class_qemu_device_1a0c99d927d7331bcea8ac24c24d4d5c9b}





### `public inline const char * get_qom_type() const` {#class_qemu_device_1a2c551bbf2c19db3cea1533a8d87bb738}





### `public inline qemu::Device get_qemu_dev()` {#class_qemu_device_1aca989d37771bb45920879aedc427ee0e}





### `public inline `[`QemuInstance`](#class_qemu_instance)` & get_qemu_inst()` {#class_qemu_device_1ab766e2e9d4607303c6763d291a7de264}





### `protected `[`QemuInstance`](#class_qemu_instance)` & m_inst` {#class_qemu_device_1a387edfdb250de6c9a988a4c6a6f96132}





### `protected qemu::Device m_dev` {#class_qemu_device_1a4ac0fabbbf9710f01c998d724cebc4fa}





### `protected bool m_realized` {#class_qemu_device_1a0df541066a42b456b116634e04b625e4}





### `protected inline void realize()` {#class_qemu_device_1abf539648b8d0742915e49479f68f0708}






# class `QemuInitiatorIface` {#class_qemu_initiator_iface}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public void initiator_customize_tlm_payload(TlmPayload & payload)` | 
`public sc_core::sc_time initiator_get_local_time()` | 
`public void initiator_set_local_time(const sc_core::sc_time &)` | 

## Members

### `public void initiator_customize_tlm_payload(TlmPayload & payload)` {#class_qemu_initiator_iface_1a946a204bd76b638f7a1026d664504725}





### `public sc_core::sc_time initiator_get_local_time()` {#class_qemu_initiator_iface_1aaaece5e4336ad8c48ca744bc4b7bda73}





### `public void initiator_set_local_time(const sc_core::sc_time &)` {#class_qemu_initiator_iface_1a751e7821825a2885435f160a6f2ff797}






# class `QemuInitiatorSignalSocket` {#class_qemu_initiator_signal_socket}

```
class QemuInitiatorSignalSocket
  : public InitiatorSignalSocket< bool >
```  

A QEMU output GPIO exposed as a InitiatorSignalSocket<bool>

This class exposes an output GPIO of a QEMU device as a InitiatorSignalSocket<bool>. It can be connected to an sc_core::sc_port<bool> or a TargetSignalSocket<bool>. Modifications to the interal QEMU GPIO will be propagated through the socket.

If this socket happens to be connected to a [QemuTargetSignalSocket](#class_qemu_target_signal_socket), the propagation is done directly within QEMU and do not go through the SystemC kernel. Note that this is only true if the GPIOs wrapped by both this socket and the remote socket lie in the same QEMU instance.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuInitiatorSignalSocket(const char * name)` | 
`public inline void init(qemu::Device dev,int gpio_idx)` | Initialize this socket with a device and a GPIO index.
`public inline void init_named(qemu::Device dev,const char * gpio_name,int gpio_idx)` | Initialize this socket with a device, a GPIO namespace, and a GPIO index.
`public inline void init_sbd(qemu::SysBusDevice sbd,int gpio_idx)` | Initialize this socket with a QEMU SysBusDevice, and a GPIO index.
`protected qemu::Gpio m_proxy` | 
`protected gs::RunOnSysC m_on_sysc` | 
`protected `[`QemuTargetSignalSocket`](#class_qemu_target_signal_socket)` * m_qemu_remote` | 
`protected inline void event_cb(bool val)` | 
`protected inline void init_qemu_to_sysc_gpio_proxy(qemu::Device & dev)` | 
`protected inline void init_internal(qemu::Device & dev)` | 

## Members

### `public inline  QemuInitiatorSignalSocket(const char * name)` {#class_qemu_initiator_signal_socket_1acb9493f45a3261d7a903e88d6f58a1bd}





### `public inline void init(qemu::Device dev,int gpio_idx)` {#class_qemu_initiator_signal_socket_1acd43da3429cc9eadb93361073990585d}

Initialize this socket with a device and a GPIO index.

This method initializes the socket using the given QEMU device and the corresponding GPIO index in this device. See the QEMU API and the device you want to wrap to know what index to use here.


#### Parameters
* `dev` The QEMU device 


* `gpio_idx` The GPIO index within the device

### `public inline void init_named(qemu::Device dev,const char * gpio_name,int gpio_idx)` {#class_qemu_initiator_signal_socket_1a68b1b26b6d5e8b2cb49eb6ee96caa424}

Initialize this socket with a device, a GPIO namespace, and a GPIO index.

This method initializes the socket using the given QEMU device and the corresponding GPIO (namespace, index) pair in this device. See the QEMU API and the device you want to wrap to know what namespace/index to use here.


#### Parameters
* `dev` The QEMU device 


* `gpio_name` The GPIO namespace within the device 


* `gpio_idx` The GPIO index within the device

### `public inline void init_sbd(qemu::SysBusDevice sbd,int gpio_idx)` {#class_qemu_initiator_signal_socket_1a3e5e067a5a071ebb71fe05e974f6ad10}

Initialize this socket with a QEMU SysBusDevice, and a GPIO index.

This method initializes the socket using the given QEMU SysBusDevice (SBD) and the corresponding GPIO index) in this SBD. See the QEMU API and the SBD you want to wrap to know what index to use here.


#### Parameters
* `sbd` The QEMU SysBusDevice 


* `gpio_idx` The GPIO index within the SBD

### `protected qemu::Gpio m_proxy` {#class_qemu_initiator_signal_socket_1a8ccd2dfdfa9d1ab408a9f0e41382901a}





### `protected gs::RunOnSysC m_on_sysc` {#class_qemu_initiator_signal_socket_1ac394597f7640c080353fbebfb8fabe47}





### `protected `[`QemuTargetSignalSocket`](#class_qemu_target_signal_socket)` * m_qemu_remote` {#class_qemu_initiator_signal_socket_1a34290c38258575bbf1eeff064d5de275}





### `protected inline void event_cb(bool val)` {#class_qemu_initiator_signal_socket_1a0535d0ed32add0ceac38a57daf1bb7d2}





### `protected inline void init_qemu_to_sysc_gpio_proxy(qemu::Device & dev)` {#class_qemu_initiator_signal_socket_1a58fc08d7dbbf4bce71b0858d89c55797}





### `protected inline void init_internal(qemu::Device & dev)` {#class_qemu_initiator_signal_socket_1a883243b799007112038ee80458db534a}






# class `QemuInitiatorSocket` {#class_qemu_initiator_socket}

```
class QemuInitiatorSocket
  : public tlm::tlm_initiator_socket< 32 >
```  

TLM-2.0 initiator socket specialisation for QEMU AddressSpace mapping.

This class is used to expose a QEMU AddressSpace object as a standard TLM-2.0 initiator socket. It creates a root memory region to map the whole address space, receives I/O accesses to it and forwards them as standard TLM-2.0 transactions.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuInitiatorSocket(const char * name,`[`QemuInitiatorIface`](#class_qemu_initiator_iface)` & initiator,`[`QemuInstance`](#class_qemu_instance)` & inst)` | 
`public inline void init(qemu::Device & dev,const char * prop)` | 
`protected `[`QemuToTlmInitiatorBridge`](#class_qemu_to_tlm_initiator_bridge)` m_bridge` | 
`protected `[`QemuInstance`](#class_qemu_instance)` & m_inst` | 
`protected `[`QemuInitiatorIface`](#class_qemu_initiator_iface)` & m_initiator` | 
`protected qemu::Device m_dev` | 
`protected gs::RunOnSysC m_on_sysc` | 
`protected qemu::MemoryRegion m_root` | 
`protected inline void init_payload(TlmPayload & trans,tlm::tlm_command command,uint64_t addr,uint64_t * val,unsigned int size)` | 
`protected inline void add_dmi_mr_alias(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` | 
`protected inline void check_dmi_hint(TlmPayload & trans)` | 
`protected inline void check_qemu_mr_hint(TlmPayload & trans)` | 
`protected inline void do_regular_access(TlmPayload & trans)` | 
`protected inline void do_debug_access(TlmPayload & trans)` | 
`protected inline MemTxResult qemu_io_access(tlm::tlm_command command,uint64_t addr,uint64_t * val,unsigned int size,MemTxAttrs attrs)` | 
`protected inline MemTxResult qemu_io_read(uint64_t addr,uint64_t * val,unsigned int size,MemTxAttrs attrs)` | 
`protected inline MemTxResult qemu_io_write(uint64_t addr,uint64_t val,unsigned int size,MemTxAttrs attrs)` | 

## Members

### `public inline  QemuInitiatorSocket(const char * name,`[`QemuInitiatorIface`](#class_qemu_initiator_iface)` & initiator,`[`QemuInstance`](#class_qemu_instance)` & inst)` {#class_qemu_initiator_socket_1a9c3c407b48b45a0aedf57b98d04daad3}





### `public inline void init(qemu::Device & dev,const char * prop)` {#class_qemu_initiator_socket_1a4f42eb0fbf1a12fbf5f234aef25c9631}





### `protected `[`QemuToTlmInitiatorBridge`](#class_qemu_to_tlm_initiator_bridge)` m_bridge` {#class_qemu_initiator_socket_1a3b0ab8d532c8988854bb02fac987f9dd}





### `protected `[`QemuInstance`](#class_qemu_instance)` & m_inst` {#class_qemu_initiator_socket_1a0bb283ba37666c24ea1a8cf25cd9e042}





### `protected `[`QemuInitiatorIface`](#class_qemu_initiator_iface)` & m_initiator` {#class_qemu_initiator_socket_1a2f0a2c2b4617fc2c3041002eab8231b8}





### `protected qemu::Device m_dev` {#class_qemu_initiator_socket_1a2fb09624914efc77883908df9380fd6f}





### `protected gs::RunOnSysC m_on_sysc` {#class_qemu_initiator_socket_1a09f2f308a197af28b6a03e5c680aa9c6}





### `protected qemu::MemoryRegion m_root` {#class_qemu_initiator_socket_1ac3258b468e57359c8926065879ebf54d}





### `protected inline void init_payload(TlmPayload & trans,tlm::tlm_command command,uint64_t addr,uint64_t * val,unsigned int size)` {#class_qemu_initiator_socket_1a1abb5631fe38d5933106065f9176bd0a}





### `protected inline void add_dmi_mr_alias(`[`DmiInfo`](#struct_qemu_instance_dmi_manager_1_1_dmi_info)` & info)` {#class_qemu_initiator_socket_1a0ea3b73c7bcd4e9b3b97f696b1978e2e}





### `protected inline void check_dmi_hint(TlmPayload & trans)` {#class_qemu_initiator_socket_1a149c4ef6437992e7a2c13a43df6d4448}





### `protected inline void check_qemu_mr_hint(TlmPayload & trans)` {#class_qemu_initiator_socket_1a165aa5b34f2fac33c7e9dcf6a1fdfaf1}





### `protected inline void do_regular_access(TlmPayload & trans)` {#class_qemu_initiator_socket_1ad94a198ddad8420a82bca1cc5ee7004e}





### `protected inline void do_debug_access(TlmPayload & trans)` {#class_qemu_initiator_socket_1ab4c37652a30d0aa0d4f8694888d4462a}





### `protected inline MemTxResult qemu_io_access(tlm::tlm_command command,uint64_t addr,uint64_t * val,unsigned int size,MemTxAttrs attrs)` {#class_qemu_initiator_socket_1a45d859f2f5d2320711a3b7b0e27316f5}





### `protected inline MemTxResult qemu_io_read(uint64_t addr,uint64_t * val,unsigned int size,MemTxAttrs attrs)` {#class_qemu_initiator_socket_1aac760d94f9779a99e41a08adef43293a}





### `protected inline MemTxResult qemu_io_write(uint64_t addr,uint64_t val,unsigned int size,MemTxAttrs attrs)` {#class_qemu_initiator_socket_1a768aba98e8234f1c04834bdb1fe2a587}






# class `QemuInstance` {#class_qemu_instance}


This class encapsulates a libqemu-cxx qemu::LibQemu instance. It handles QEMU parameters and instance initialization.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuInstance(LibLoader & loader,Target t)` | 
`public  QemuInstance(const `[`QemuInstance`](#class_qemu_instance)` &) = delete` | 
`public  QemuInstance(`[`QemuInstance`](#class_qemu_instance)` &&) = default` | 
`public inline virtual  ~QemuInstance()` | 
`public inline void set_tcg_mode(TcgMode m)` | Set the desired TCG mode for this instance.
`public inline void set_icount_mode(IcountMode m,int mips_shift)` | Set the desired icount mode for this instance.
`public inline void init()` | Initialize the QEMU instance.
`public inline bool is_inited() const` | Returns true if the instance is initialized.
`public inline qemu::LibQemu & get()` | Returns the underlying qemu::LibQemu instance.
`public inline `[`LockedQemuInstanceDmiManager`](#class_locked_qemu_instance_dmi_manager)` get_dmi_manager()` | Returns the locked [QemuInstanceDmiManager](#class_qemu_instance_dmi_manager) instance.
`protected qemu::LibQemu m_inst` | 
`protected `[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` m_dmi_mgr` | 
`protected TcgMode m_tcg_mode` | 
`protected IcountMode m_icount_mode` | 
`protected int m_icount_mips` | 
`protected inline void push_default_args()` | 
`protected inline void push_icount_mode_args()` | 
`protected inline void push_tcg_mode_args()` | 

## Members

### `public inline  QemuInstance(LibLoader & loader,Target t)` {#class_qemu_instance_1a06a4756cf2ec3bea0f8403c8932f2f08}





### `public  QemuInstance(const `[`QemuInstance`](#class_qemu_instance)` &) = delete` {#class_qemu_instance_1addc47e7fc6dfbccc0779a46d86482e5e}





### `public  QemuInstance(`[`QemuInstance`](#class_qemu_instance)` &&) = default` {#class_qemu_instance_1a3ed4c26d99f6a9e0b72090fb4688b3b4}





### `public inline virtual  ~QemuInstance()` {#class_qemu_instance_1a54c34fb8c6d1051b50c188a0be39795d}





### `public inline void set_tcg_mode(TcgMode m)` {#class_qemu_instance_1a4a49b0bffe623f542d5bbaf8bbe84771}

Set the desired TCG mode for this instance.

This method is called by CPU instances to specify the desired TCG mode according to the synchronization policy in use. All CPUs should use the same mode (meaning they should all use synchronization policies compatible one with the other).

This method should be called before the instance is initialized.

### `public inline void set_icount_mode(IcountMode m,int mips_shift)` {#class_qemu_instance_1ac2d9036b0f631e6e580a3bcd0c88b58a}

Set the desired icount mode for this instance.

This method is called by CPU instances to specify the desired icount mode according to the synchronization policy in use. All CPUs should use the same mode.

This method should be called before the instance is initialized.


#### Parameters
* `m` The desired icount mode 


* `mips_shift` The QEMU icount shift parameter. It sets the virtual time an instruction takes to execute to 2^(mips_shift) ns.

### `public inline void init()` {#class_qemu_instance_1aea304500ffec19a1086e0108c31a4426}

Initialize the QEMU instance.

Initialize the QEMU instance with the set TCG and icount mode. If the TCG mode hasn't been set, it defaults to TCG_SINGLE. If icount mode hasn't been set, it defaults to ICOUNT_OFF.

The instance should not already be initialized when calling this method.

### `public inline bool is_inited() const` {#class_qemu_instance_1ab9f0e5c6dcd823443e59bbf7aa0515ca}

Returns true if the instance is initialized.



### `public inline qemu::LibQemu & get()` {#class_qemu_instance_1ab626a8757f63cfa1d146ee95a0cbc32f}

Returns the underlying qemu::LibQemu instance.

Returns the underlying qemu::LibQemu instance. If the instance hasn't been initialized, init is called just before returning the instance.

### `public inline `[`LockedQemuInstanceDmiManager`](#class_locked_qemu_instance_dmi_manager)` get_dmi_manager()` {#class_qemu_instance_1a2feefb4e95abfb329d5604d2d811ee53}

Returns the locked [QemuInstanceDmiManager](#class_qemu_instance_dmi_manager) instance.

Note: we rely on RVO here so no copy happen on return (this is enforced by the fact that the [LockedQemuInstanceDmiManager](#class_locked_qemu_instance_dmi_manager) copy constructor is deleted).

### `protected qemu::LibQemu m_inst` {#class_qemu_instance_1abdec2b015d619ce9bd9f2e69e85d075e}





### `protected `[`QemuInstanceDmiManager`](#class_qemu_instance_dmi_manager)` m_dmi_mgr` {#class_qemu_instance_1a2912c078202e6e11aa41ad1d1f0329da}





### `protected TcgMode m_tcg_mode` {#class_qemu_instance_1a84f38e3ff1194c85d4f72af6bcb253b2}





### `protected IcountMode m_icount_mode` {#class_qemu_instance_1a1bd7f393e74a5cc20ffe5a504f44c4e3}





### `protected int m_icount_mips` {#class_qemu_instance_1aecf88d1ce2fdff97c59c7bf670c1b028}





### `protected inline void push_default_args()` {#class_qemu_instance_1af4ec2225e6a42e945e627fcbaeaba9ca}





### `protected inline void push_icount_mode_args()` {#class_qemu_instance_1a18dc3d2ed275456c6ca9095014fcbae8}





### `protected inline void push_tcg_mode_args()` {#class_qemu_instance_1ae2f2f61e2316b5ecfe4597180aa59fc0}






# class `QemuInstanceIcountModeMismatchException` {#class_qemu_instance_icount_mode_mismatch_exception}

```
class QemuInstanceIcountModeMismatchException
  : public QboxException
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuInstanceIcountModeMismatchException()` | 
`public inline virtual  ~QemuInstanceIcountModeMismatchException()` | 

## Members

### `public inline  QemuInstanceIcountModeMismatchException()` {#class_qemu_instance_icount_mode_mismatch_exception_1a44d58df36eeb7718026dd9f8206c2663}





### `public inline virtual  ~QemuInstanceIcountModeMismatchException()` {#class_qemu_instance_icount_mode_mismatch_exception_1a36feca73228611aa1d0738353fcd7351}






# class `QemuInstanceManager` {#class_qemu_instance_manager}


QEMU instance manager class.

This class manages QEMU instances. It allows to create instances using the same library loader, thus allowing multiple instances of the same library being loaded.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuInstanceManager()` | Construct a [QemuInstanceManager](#class_qemu_instance_manager). The manager will use the default library loader provided by libqemu-cxx.
`public inline  QemuInstanceManager(LibLoader * loader)` | Construct a [QemuInstanceManager](#class_qemu_instance_manager) by providing a custom library loader.
`public inline `[`QemuInstance`](#class_qemu_instance)` & new_instance(Target t)` | Returns a new QEMU instance for target t.
`public inline virtual  ~QemuInstanceManager()` | 
`protected LibLoader * m_loader` | 
`protected std::vector< `[`QemuInstance`](#class_qemu_instance)` > m_insts` | 

## Members

### `public inline  QemuInstanceManager()` {#class_qemu_instance_manager_1a85b375bf686a1c5e0ce973bd1d5e263d}

Construct a [QemuInstanceManager](#class_qemu_instance_manager). The manager will use the default library loader provided by libqemu-cxx.



### `public inline  QemuInstanceManager(LibLoader * loader)` {#class_qemu_instance_manager_1a7766d7152a27ccd3bccb8aac53d9b851}

Construct a [QemuInstanceManager](#class_qemu_instance_manager) by providing a custom library loader.

#### Parameters
* `loader` The custom loader

### `public inline `[`QemuInstance`](#class_qemu_instance)` & new_instance(Target t)` {#class_qemu_instance_manager_1aaa7d4f85b7df086d210b23ac1bf9ba72}

Returns a new QEMU instance for target t.



### `public inline virtual  ~QemuInstanceManager()` {#class_qemu_instance_manager_1af5140a1b889ffa83b21c8b9355919975}





### `protected LibLoader * m_loader` {#class_qemu_instance_manager_1adc15b081205afdcfbf013ef5403fefdc}





### `protected std::vector< `[`QemuInstance`](#class_qemu_instance)` > m_insts` {#class_qemu_instance_manager_1a149077cc8d03a7c0ea39065cc22aacdf}






# class `QemuInstanceTcgModeMismatchException` {#class_qemu_instance_tcg_mode_mismatch_exception}

```
class QemuInstanceTcgModeMismatchException
  : public QboxException
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuInstanceTcgModeMismatchException()` | 
`public inline virtual  ~QemuInstanceTcgModeMismatchException()` | 

## Members

### `public inline  QemuInstanceTcgModeMismatchException()` {#class_qemu_instance_tcg_mode_mismatch_exception_1a88f754233c99b23087720b89ced34fa7}





### `public inline virtual  ~QemuInstanceTcgModeMismatchException()` {#class_qemu_instance_tcg_mode_mismatch_exception_1a020904e17b7c20c6f3bb21e605b2de66}






# class `QemuMrHintTlmExtension` {#class_qemu_mr_hint_tlm_extension}

```
class QemuMrHintTlmExtension
  : public tlm::tlm_extension< QemuMrHintTlmExtension >
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  QemuMrHintTlmExtension() = default` | 
`public  QemuMrHintTlmExtension(const `[`QemuMrHintTlmExtension`](#class_qemu_mr_hint_tlm_extension)` &) = default` | 
`public inline  QemuMrHintTlmExtension(qemu::MemoryRegion mr,uint64_t offset)` | 
`public inline virtual tlm_extension_base * clone() const` | 
`public inline virtual void copy_from(tlm_extension_base const & ext)` | 
`public inline qemu::MemoryRegion get_mr() const` | 
`public inline uint64_t get_offset() const` | 

## Members

### `public  QemuMrHintTlmExtension() = default` {#class_qemu_mr_hint_tlm_extension_1aeb424f16b8189dcb76284abc28311703}





### `public  QemuMrHintTlmExtension(const `[`QemuMrHintTlmExtension`](#class_qemu_mr_hint_tlm_extension)` &) = default` {#class_qemu_mr_hint_tlm_extension_1a0046d94a670c87735cb96bb2f6c9cae8}





### `public inline  QemuMrHintTlmExtension(qemu::MemoryRegion mr,uint64_t offset)` {#class_qemu_mr_hint_tlm_extension_1a26b8e387ab06967f1cd4c34c375de464}





### `public inline virtual tlm_extension_base * clone() const` {#class_qemu_mr_hint_tlm_extension_1a9a143feba21534bdb49eda1b90fe16b4}





### `public inline virtual void copy_from(tlm_extension_base const & ext)` {#class_qemu_mr_hint_tlm_extension_1a2aac79a38e84ad79670d98485d3f565b}





### `public inline qemu::MemoryRegion get_mr() const` {#class_qemu_mr_hint_tlm_extension_1a26956aafa57ce3ad652aeab864b63e10}





### `public inline uint64_t get_offset() const` {#class_qemu_mr_hint_tlm_extension_1aed8300f5a00be6067f5e3b8342939d21}






# class `QemuRiscvSifiveClint` {#class_qemu_riscv_sifive_clint}

```
class QemuRiscvSifiveClint
  : public QemuDevice
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public cci::cci_param< unsigned int > p_num_harts` | 
`public cci::cci_param< uint64_t > p_sip_base` | 
`public cci::cci_param< uint64_t > p_timecmp_base` | 
`public cci::cci_param< uint64_t > p_time_base` | 
`public cci::cci_param< bool > p_provide_rdtime` | 
`public cci::cci_param< uint64_t > p_aperture_size` | 
`public `[`QemuTargetSocket`](#class_qemu_target_socket)` socket` | 
`public inline  QemuRiscvSifiveClint(sc_core::sc_module_name nm,`[`QemuInstance`](#class_qemu_instance)` & inst)` | 
`public inline virtual void before_end_of_elaboration()` | 
`public inline virtual void end_of_elaboration()` | 
`protected uint64_t m_aperture_size` | 
`protected int m_num_harts` | 

## Members

### `public cci::cci_param< unsigned int > p_num_harts` {#class_qemu_riscv_sifive_clint_1abf2a42285dae22775c57f50ca07b6944}





### `public cci::cci_param< uint64_t > p_sip_base` {#class_qemu_riscv_sifive_clint_1a9594d4e8f3b6da91396d70d17480bee6}





### `public cci::cci_param< uint64_t > p_timecmp_base` {#class_qemu_riscv_sifive_clint_1a49f08b8222c6fb06c69c8ff83fbe4654}





### `public cci::cci_param< uint64_t > p_time_base` {#class_qemu_riscv_sifive_clint_1a92e89b5635ce1734c39165fff450bfa3}





### `public cci::cci_param< bool > p_provide_rdtime` {#class_qemu_riscv_sifive_clint_1a633ad77d0316b9f9b0a130cc17e45358}





### `public cci::cci_param< uint64_t > p_aperture_size` {#class_qemu_riscv_sifive_clint_1a464388f3ba8af2adf2a262a5f47fd18c}





### `public `[`QemuTargetSocket`](#class_qemu_target_socket)` socket` {#class_qemu_riscv_sifive_clint_1a09dc27a52426c26297f82719ef9dee0c}





### `public inline  QemuRiscvSifiveClint(sc_core::sc_module_name nm,`[`QemuInstance`](#class_qemu_instance)` & inst)` {#class_qemu_riscv_sifive_clint_1aaeb66aad080b00998362a87402c246fb}





### `public inline virtual void before_end_of_elaboration()` {#class_qemu_riscv_sifive_clint_1ad6d2813e385f8f6d0d396e6292113a13}





### `public inline virtual void end_of_elaboration()` {#class_qemu_riscv_sifive_clint_1a93daa0d06d6229faea7c4ffe6aa55ccf}





### `protected uint64_t m_aperture_size` {#class_qemu_riscv_sifive_clint_1ab7f7c8f3be48b06813d2847331435e0a}





### `protected int m_num_harts` {#class_qemu_riscv_sifive_clint_1ac75501945c582c32662428b48c000556}






# class `QemuRiscvSifivePlic` {#class_qemu_riscv_sifive_plic}

```
class QemuRiscvSifivePlic
  : public QemuDevice
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public cci::cci_param< unsigned int > p_num_sources` | 
`public cci::cci_param< unsigned int > p_num_priorities` | 
`public cci::cci_param< uint64_t > p_priority_base` | 
`public cci::cci_param< uint64_t > p_pending_base` | 
`public cci::cci_param< uint64_t > p_enable_base` | 
`public cci::cci_param< uint64_t > p_enable_stride` | 
`public cci::cci_param< uint64_t > p_context_base` | 
`public cci::cci_param< uint64_t > p_context_stride` | 
`public cci::cci_param< uint64_t > p_aperture_size` | 
`public cci::cci_param< std::string > p_hart_config` | 
`public `[`QemuTargetSocket`](#class_qemu_target_socket)` socket` | 
`public sc_core::sc_vector< `[`QemuTargetSignalSocket`](#class_qemu_target_signal_socket)` > irq_in` | 
`public inline  QemuRiscvSifivePlic(sc_core::sc_module_name nm,`[`QemuInstance`](#class_qemu_instance)` & inst)` | 
`public inline virtual void before_end_of_elaboration()` | 
`public inline virtual void end_of_elaboration()` | 

## Members

### `public cci::cci_param< unsigned int > p_num_sources` {#class_qemu_riscv_sifive_plic_1aacde1beaac8bac63ee81c63a8c173eff}





### `public cci::cci_param< unsigned int > p_num_priorities` {#class_qemu_riscv_sifive_plic_1abb1f44a4fb8bd9a03b920e399ef9d5d9}





### `public cci::cci_param< uint64_t > p_priority_base` {#class_qemu_riscv_sifive_plic_1acb5f8e04119838357a794c120d1b219a}





### `public cci::cci_param< uint64_t > p_pending_base` {#class_qemu_riscv_sifive_plic_1a0bb1cafc6e6d02d482ac2876d8f611e5}





### `public cci::cci_param< uint64_t > p_enable_base` {#class_qemu_riscv_sifive_plic_1a63751c75c6f9dec17f1fcfc8133f1460}





### `public cci::cci_param< uint64_t > p_enable_stride` {#class_qemu_riscv_sifive_plic_1a6935d0728e8487bb1ae00879f8311395}





### `public cci::cci_param< uint64_t > p_context_base` {#class_qemu_riscv_sifive_plic_1a3adfe1ec7784977caca703592d091358}





### `public cci::cci_param< uint64_t > p_context_stride` {#class_qemu_riscv_sifive_plic_1a8c59520826f156adb2e0eae985b3ec91}





### `public cci::cci_param< uint64_t > p_aperture_size` {#class_qemu_riscv_sifive_plic_1a0123d846f45d1d6a489a43da47c61c7c}





### `public cci::cci_param< std::string > p_hart_config` {#class_qemu_riscv_sifive_plic_1add94a87170c72e6d97860c545ae9c450}





### `public `[`QemuTargetSocket`](#class_qemu_target_socket)` socket` {#class_qemu_riscv_sifive_plic_1ac9cfa98f0ef240d7c78d77f2cce55308}





### `public sc_core::sc_vector< `[`QemuTargetSignalSocket`](#class_qemu_target_signal_socket)` > irq_in` {#class_qemu_riscv_sifive_plic_1a347e1b14291787e8ee1f98be750c0458}





### `public inline  QemuRiscvSifivePlic(sc_core::sc_module_name nm,`[`QemuInstance`](#class_qemu_instance)` & inst)` {#class_qemu_riscv_sifive_plic_1a66dc2adca07f31e53a3667e91d0bdecd}





### `public inline virtual void before_end_of_elaboration()` {#class_qemu_riscv_sifive_plic_1a6819f0144d798d4ef3aaec8c9823c996}





### `public inline virtual void end_of_elaboration()` {#class_qemu_riscv_sifive_plic_1adb76c0dd6d29bf738d53a0bca3681d49}






# class `QemuTargetSignalSocket` {#class_qemu_target_signal_socket}

```
class QemuTargetSignalSocket
  : public TargetSignalSocket< bool >
```  

A QEMU input GPIO exposed as a TargetSignalSocket<bool>

This class exposes an input GPIO of a QEMU device as a TargetSignalSocket<bool>. It can be connected to an sc_core::sc_port<bool> or a TargetInitiatorSocket<bool>. Modifications to this socket will be reported to the wrapped GPIO.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuTargetSignalSocket(const char * name)` | 
`public inline void init(qemu::Device dev,int gpio_idx)` | Initialize this socket with a device and a GPIO index.
`public inline void init_named(qemu::Device dev,const char * gpio_name,int gpio_idx)` | Initialize this socket with a device, a GPIO namespace, and a GPIO index.
`public inline qemu::Gpio get_gpio()` | Returns the GPIO wrapped by this socket.
`protected qemu::Gpio m_gpio_in` | 
`protected inline void value_changed_cb(const bool & val)` | 
`protected inline void init_with_gpio(qemu::Gpio gpio)` | 

## Members

### `public inline  QemuTargetSignalSocket(const char * name)` {#class_qemu_target_signal_socket_1a8343b3d2e50348aae790193fea24791d}





### `public inline void init(qemu::Device dev,int gpio_idx)` {#class_qemu_target_signal_socket_1abe5fc8983c9f91d80ea3c02496f9c37f}

Initialize this socket with a device and a GPIO index.

This method initializes the socket using the given QEMU device and the corresponding GPIO index in this device. See the QEMU API and the device you want to wrap to know what index to use here.


#### Parameters
* `dev` The QEMU device 


* `gpio_idx` The GPIO index within the device

### `public inline void init_named(qemu::Device dev,const char * gpio_name,int gpio_idx)` {#class_qemu_target_signal_socket_1a2644e2c4415bf470f372db1a098863d4}

Initialize this socket with a device, a GPIO namespace, and a GPIO index.

This method initializes the socket using the given QEMU device and the corresponding GPIO (namespace, index) pair in this device. See the QEMU API and the device you want to wrap to know what namespace/index to use here.


#### Parameters
* `dev` The QEMU device 


* `gpio_name` The GPIO namespace within the device 


* `gpio_idx` The GPIO index within the device

### `public inline qemu::Gpio get_gpio()` {#class_qemu_target_signal_socket_1ad8b0355c72e067f99601dc08930900a8}

Returns the GPIO wrapped by this socket.

#### Returns
the GPIO wrapped by this socket

### `protected qemu::Gpio m_gpio_in` {#class_qemu_target_signal_socket_1a5093037ebbd476dd2736466fed408870}





### `protected inline void value_changed_cb(const bool & val)` {#class_qemu_target_signal_socket_1a7b15857066470a0e2e22d62c00615975}





### `protected inline void init_with_gpio(qemu::Gpio gpio)` {#class_qemu_target_signal_socket_1ab4c07749bc667224d1b5450115a9e19d}






# class `QemuTargetSocket` {#class_qemu_target_socket}

```
class QemuTargetSocket
  : public tlm::tlm_target_socket< 32 >
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  QemuTargetSocket(const char * name,`[`QemuInstance`](#class_qemu_instance)` & inst)` | 
`public inline void init(qemu::SysBusDevice sbd,int mmio_idx)` | 
`protected `[`TlmTargetToQemuBridge`](#class_tlm_target_to_qemu_bridge)` m_bridge` | 
`protected `[`QemuInstance`](#class_qemu_instance)` & m_inst` | 
`protected qemu::SysBusDevice m_sbd` | 

## Members

### `public inline  QemuTargetSocket(const char * name,`[`QemuInstance`](#class_qemu_instance)` & inst)` {#class_qemu_target_socket_1af0a9dd8fe8ccb8a3facd37f0b686fd94}





### `public inline void init(qemu::SysBusDevice sbd,int mmio_idx)` {#class_qemu_target_socket_1af682ea90a9d323e0c38213b82ee8d7d1}





### `protected `[`TlmTargetToQemuBridge`](#class_tlm_target_to_qemu_bridge)` m_bridge` {#class_qemu_target_socket_1a44a87be77c71f1f572aa6ca87006f1bd}





### `protected `[`QemuInstance`](#class_qemu_instance)` & m_inst` {#class_qemu_target_socket_1ae600662a9f3508ac6f4330dfde9c7fa7}





### `protected qemu::SysBusDevice m_sbd` {#class_qemu_target_socket_1a9d3578561e67422d5c86a82517826b85}






# class `QemuToTlmInitiatorBridge` {#class_qemu_to_tlm_initiator_bridge}

```
class QemuToTlmInitiatorBridge
  : public tlm::tlm_bw_transport_if<>
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload & trans,tlm::tlm_phase & phase,sc_core::sc_time & t)` | 
`public inline virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range,sc_dt::uint64 end_range)` | 

## Members

### `public inline virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload & trans,tlm::tlm_phase & phase,sc_core::sc_time & t)` {#class_qemu_to_tlm_initiator_bridge_1a82e4da367addfffafdd5a5acb6857fda}





### `public inline virtual void invalidate_direct_mem_ptr(sc_dt::uint64 start_range,sc_dt::uint64 end_range)` {#class_qemu_to_tlm_initiator_bridge_1add5e596b2b3f66d74e68fcb4d4a48cc6}






# class `QemuUart16550` {#class_qemu_uart16550}

```
class QemuUart16550
  : public QemuDevice
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`QemuTargetSocket`](#class_qemu_target_socket)` socket` | 
`public `[`QemuInitiatorSignalSocket`](#class_qemu_initiator_signal_socket)` irq_out` | 
`public inline  QemuUart16550(const sc_core::sc_module_name & n,`[`QemuInstance`](#class_qemu_instance)` & inst)` | 
`public inline virtual void before_end_of_elaboration()` | 
`public inline virtual void end_of_elaboration()` | 
`protected qemu::Chardev m_chardev` | 
`protected cci::cci_param< unsigned int > p_baudbase` | 
`protected cci::cci_param< unsigned int > p_regshift` | 

## Members

### `public `[`QemuTargetSocket`](#class_qemu_target_socket)` socket` {#class_qemu_uart16550_1a9ccf314514efb539cde7f280e9c0474c}





### `public `[`QemuInitiatorSignalSocket`](#class_qemu_initiator_signal_socket)` irq_out` {#class_qemu_uart16550_1abbb7e71b9963c2fe10e954f96e34f5e7}





### `public inline  QemuUart16550(const sc_core::sc_module_name & n,`[`QemuInstance`](#class_qemu_instance)` & inst)` {#class_qemu_uart16550_1a04f1edd65e326617a4ef13cbde2dc592}





### `public inline virtual void before_end_of_elaboration()` {#class_qemu_uart16550_1a7adafa952f325d172d82c944202c64ed}





### `public inline virtual void end_of_elaboration()` {#class_qemu_uart16550_1afa923a8d0a8489bfdc25eb34db5ba017}





### `protected qemu::Chardev m_chardev` {#class_qemu_uart16550_1a5fd701e71c879c9eb510d04d8bfe558a}





### `protected cci::cci_param< unsigned int > p_baudbase` {#class_qemu_uart16550_1a9c75b01e7f9b15469749e742ccacced5}





### `protected cci::cci_param< unsigned int > p_regshift` {#class_qemu_uart16550_1a7cfe4ecee1348b0255848ae397fffaa0}






# class `TlmTargetToQemuBridge` {#class_tlm_target_to_qemu_bridge}

```
class TlmTargetToQemuBridge
  : public tlm::tlm_fw_transport_if<>
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline void init(qemu::SysBusDevice sbd,int mmio_idx)` | 
`public inline virtual void b_transport(TlmPayload & trans,sc_core::sc_time & t)` | 
`public inline virtual tlm::tlm_sync_enum nb_transport_fw(TlmPayload & trans,tlm::tlm_phase & phase,sc_core::sc_time & t)` | 
`public inline virtual bool get_direct_mem_ptr(TlmPayload & trans,tlm::tlm_dmi & dmi_data)` | 
`public inline virtual unsigned int transport_dbg(TlmPayload & trans)` | 
`protected qemu::MemoryRegion m_mr` | 
`protected inline qemu::Cpu push_current_cpu(TlmPayload & trans)` | 
`protected inline void pop_current_cpu(qemu::Cpu cpu)` | 

## Members

### `public inline void init(qemu::SysBusDevice sbd,int mmio_idx)` {#class_tlm_target_to_qemu_bridge_1ae3e697a843c6e576aaccc8907e84c3c8}





### `public inline virtual void b_transport(TlmPayload & trans,sc_core::sc_time & t)` {#class_tlm_target_to_qemu_bridge_1ac70c4eb6218f718434d16434edfb0670}





### `public inline virtual tlm::tlm_sync_enum nb_transport_fw(TlmPayload & trans,tlm::tlm_phase & phase,sc_core::sc_time & t)` {#class_tlm_target_to_qemu_bridge_1ae118846416f607da9430f63b0196d09e}





### `public inline virtual bool get_direct_mem_ptr(TlmPayload & trans,tlm::tlm_dmi & dmi_data)` {#class_tlm_target_to_qemu_bridge_1a420bc45c7365d5a4587afe4e4f6a190b}





### `public inline virtual unsigned int transport_dbg(TlmPayload & trans)` {#class_tlm_target_to_qemu_bridge_1af49211285bf5c1f298f292edf8d62086}





### `protected qemu::MemoryRegion m_mr` {#class_tlm_target_to_qemu_bridge_1aa9f0df6f52ac1c19567814370b632890}





### `protected inline qemu::Cpu push_current_cpu(TlmPayload & trans)` {#class_tlm_target_to_qemu_bridge_1a1f8bd266380e8462dc95fe7428682b7a}





### `protected inline void pop_current_cpu(qemu::Cpu cpu)` {#class_tlm_target_to_qemu_bridge_1a25e053a63ed432a8b0ac8ccc2c9ca293}






