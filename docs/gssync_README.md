# namespace `gs`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`namespace `[``SyncPolicy``](#namespacegs_1_1_sync_policy)    | 
`class `[``async_event``](#classgs_1_1async__event)    | 
`class `[``RunOnSysC``](#classgs_1_1_run_on_sys_c)    | 
`class `[``global_pause``](#classgs_1_1global__pause)    | 
`class `[``InLineSync``](#classgs_1_1_in_line_sync)    | 
`class `[``semaphore``](#classgs_1_1semaphore)    | 
`class `[``tlm_quantumkeeper_extended``](#classgs_1_1tlm__quantumkeeper__extended)    | 
`class `[``tlm_quantumkeeper_multi_quantum``](#classgs_1_1tlm__quantumkeeper__multi__quantum)    | 
`class `[``tlm_quantumkeeper_multi_rolling``](#classgs_1_1tlm__quantumkeeper__multi__rolling)    | 
`class `[``tlm_quantumkeeper_multithread``](#classgs_1_1tlm__quantumkeeper__multithread)    | 

# namespace `SyncPolicy`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# class `async_event` {#classgs_1_1async__event}

```
class async_event
  : public sc_core::sc_prim_channel
  : public sc_core::sc_event
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  async_event(bool start_attached)` | 
`public inline void async_notify()` | 
`public inline void notify(sc_core::sc_time delay)` | 
`public inline void async_attach_suspending()` | 
`public inline void async_detach_suspending()` | 
`public inline void enable_attach_suspending(bool e)` | 

## Members

### `public inline  async_event(bool start_attached)` {#classgs_1_1async__event_1ab31d1d2244138584f2295f08b609e22f}





### `public inline void async_notify()` {#classgs_1_1async__event_1a2f72dcaf0067f0a130a616e969a3cd1f}





### `public inline void notify(sc_core::sc_time delay)` {#classgs_1_1async__event_1a65895bce9a12abcc709044859f8100ee}





### `public inline void async_attach_suspending()` {#classgs_1_1async__event_1afdf4ca0a52223003d0e6cb4bb2273935}





### `public inline void async_detach_suspending()` {#classgs_1_1async__event_1ab43c758e4ae1315666715c5a74395e42}





### `public inline void enable_attach_suspending(bool e)` {#classgs_1_1async__event_1a8c5b58423522ac41702c62921f8e510d}






# class `RunOnSysC` {#classgs_1_1_run_on_sys_c}

```
class RunOnSysC
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`class `[``AsyncJob``](#classgs_1_1_run_on_sys_c_1_1_async_job)        | 
`public inline  RunOnSysC(const sc_core::sc_module_name & n)` | 
`public inline void end_of_simulation()` | 
`public inline void fork_on_systemc(std::function< void()> job_entry)` | 
`public inline void run_on_sysc(std::function< void()> job_entry,bool complete)` | 

## Members

### `class `[``AsyncJob``](#classgs_1_1_run_on_sys_c_1_1_async_job) {#classgs_1_1_run_on_sys_c_1_1_async_job}




### `public inline  RunOnSysC(const sc_core::sc_module_name & n)` {#classgs_1_1_run_on_sys_c_1a0fa4e8a433c9d729d62159aebe18b7d9}





### `public inline void end_of_simulation()` {#classgs_1_1_run_on_sys_c_1ae41a5f6ac16982d9a1c4c8a4c9de2434}





### `public inline void fork_on_systemc(std::function< void()> job_entry)` {#classgs_1_1_run_on_sys_c_1a04bab8a73f3433237397e48ab6f0320c}





### `public inline void run_on_sysc(std::function< void()> job_entry,bool complete)` {#classgs_1_1_run_on_sys_c_1ab4dea8875e98b59c59d16b087322b4d9}






# class `AsyncJob` {#classgs_1_1_run_on_sys_c_1_1_async_job}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public bool accepted` | 
`public bool cancelled` | 
`public inline  AsyncJob(std::function< void()> && job)` | 
`public inline  AsyncJob(std::function< void()> & job)` | 
`public  AsyncJob() = delete` | 
`public  AsyncJob(const AsyncJob &) = delete` | 
`public inline std::packaged_task< void()> & get_job()` | 
`public inline void operator()()` | 

## Members

### `public bool accepted` {#classgs_1_1_run_on_sys_c_1_1_async_job_1a737e0b9b1d257f12c691a5a68c2f2cf8}





### `public bool cancelled` {#classgs_1_1_run_on_sys_c_1_1_async_job_1ad7a3596e0882d5d8d4b57204bee55416}





### `public inline  AsyncJob(std::function< void()> && job)` {#classgs_1_1_run_on_sys_c_1_1_async_job_1a040017fbe529e39455f5c7819d8b0129}





### `public inline  AsyncJob(std::function< void()> & job)` {#classgs_1_1_run_on_sys_c_1_1_async_job_1abd362a89bcb96a60e597ef7f8eeb456f}





### `public  AsyncJob() = delete` {#classgs_1_1_run_on_sys_c_1_1_async_job_1a17866f1154976b99f898a83bdac04371}





### `public  AsyncJob(const AsyncJob &) = delete` {#classgs_1_1_run_on_sys_c_1_1_async_job_1a8561a12eb56e3ea815cb9e39aa88b4b2}





### `public inline std::packaged_task< void()> & get_job()` {#classgs_1_1_run_on_sys_c_1_1_async_job_1affdefd8fb15d69041958187cdea6a91c}





### `public inline void operator()()` {#classgs_1_1_run_on_sys_c_1_1_async_job_1a95331e2a097a9216adb3aa89c77787c4}






# class `global_pause` {#classgs_1_1global__pause}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  global_pause()` | 
`public  global_pause(const `[`global_pause`](#classgs_1_1global__pause)` &) = delete` | 
`public void suspendable()` | 
`public void unsuspendable()` | 
`public void unsuspend_all()` | 
`public void suspend_all()` | 
`public bool attach_suspending(sc_core::sc_prim_channel * p)` | 
`public bool detach_suspending(sc_core::sc_prim_channel * p)` | 
`public void async_wakeup()` | 

## Members

### `public  global_pause()` {#classgs_1_1global__pause_1a56303be20d575a4a9155499ad15f7f4c}





### `public  global_pause(const `[`global_pause`](#classgs_1_1global__pause)` &) = delete` {#classgs_1_1global__pause_1a10c4ff18519a5566b4f41aac14b77b16}





### `public void suspendable()` {#classgs_1_1global__pause_1a1bed96fa6e474e2c48eb6822be8f22c6}





### `public void unsuspendable()` {#classgs_1_1global__pause_1a3e92fc320b9ef17b9e01443ee61d700f}





### `public void unsuspend_all()` {#classgs_1_1global__pause_1ab9c1c8b4bd45c54032364206714dd22a}





### `public void suspend_all()` {#classgs_1_1global__pause_1a46eb6387097d9978f312e3f08f018f7b}





### `public bool attach_suspending(sc_core::sc_prim_channel * p)` {#classgs_1_1global__pause_1a6f43c976b0be71d224b675c8af585eb0}





### `public bool detach_suspending(sc_core::sc_prim_channel * p)` {#classgs_1_1global__pause_1a396b85317efc98df0a87fd4e1ca688de}





### `public void async_wakeup()` {#classgs_1_1global__pause_1ad56e4feb46e123155540c7730432549c}






# class `InLineSync` {#classgs_1_1_in_line_sync}

```
class InLineSync
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public tlm_utils::simple_target_socket< `[`InLineSync`](#classgs_1_1_in_line_sync)` > target_socket` | 
`public tlm_utils::simple_initiator_socket< `[`InLineSync`](#classgs_1_1_in_line_sync)` > initiator_socket` | 
`public  SC_HAS_PROCESS(`[`InLineSync`](#classgs_1_1_in_line_sync)`)` | 
`public inline  InLineSync(const sc_core::sc_module_name & name)` | 
`public inline void b_transport(tlm::tlm_generic_payload & trans,sc_core::sc_time & delay)` | 
`public inline void get_direct_mem_ptr(tlm::tlm_generic_payload & trans,tlm::tlm_dmi & dmi_data)` | 
`public inline void transport_dgb(tlm::tlm_generic_payload & trans)` | 
`public inline void invalidate_direct_mem_ptr(sc_dt::uint64 start_range,sc_dt::uint64 end_range)` | 

## Members

### `public tlm_utils::simple_target_socket< `[`InLineSync`](#classgs_1_1_in_line_sync)` > target_socket` {#classgs_1_1_in_line_sync_1a38e1f011a9029fd9e70794a983749024}





### `public tlm_utils::simple_initiator_socket< `[`InLineSync`](#classgs_1_1_in_line_sync)` > initiator_socket` {#classgs_1_1_in_line_sync_1a8706e294a3d803aea6109b9bfa513fd8}





### `public  SC_HAS_PROCESS(`[`InLineSync`](#classgs_1_1_in_line_sync)`)` {#classgs_1_1_in_line_sync_1a70edaddb78d3d0a8266dd2a7a1952111}





### `public inline  InLineSync(const sc_core::sc_module_name & name)` {#classgs_1_1_in_line_sync_1a9982181a0cb92e6546ce6d7e445aac7f}





### `public inline void b_transport(tlm::tlm_generic_payload & trans,sc_core::sc_time & delay)` {#classgs_1_1_in_line_sync_1a587b99619bec1de7c4e30a8992983eaa}





### `public inline void get_direct_mem_ptr(tlm::tlm_generic_payload & trans,tlm::tlm_dmi & dmi_data)` {#classgs_1_1_in_line_sync_1a446f67160e0ec0032e7085aec08ca3f6}





### `public inline void transport_dgb(tlm::tlm_generic_payload & trans)` {#classgs_1_1_in_line_sync_1a053cfd9a4e0122baf365488dd60270ea}





### `public inline void invalidate_direct_mem_ptr(sc_dt::uint64 start_range,sc_dt::uint64 end_range)` {#classgs_1_1_in_line_sync_1a23fdb6763d9520971bd57cbf67329ddc}






# class `semaphore` {#classgs_1_1semaphore}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline void notify()` | 
`public inline void wait()` | 
`public inline bool try_wait()` | 
`public  semaphore() = default` | 
`public inline  ~semaphore()` | 

## Members

### `public inline void notify()` {#classgs_1_1semaphore_1a984a2ac62bbdedeb0748ee0ed139f7e3}





### `public inline void wait()` {#classgs_1_1semaphore_1a9ba63faa984ff68f40d735198d984d2a}





### `public inline bool try_wait()` {#classgs_1_1semaphore_1a35c998fd18d93d80af162fd89761747a}





### `public  semaphore() = default` {#classgs_1_1semaphore_1a60acb8586d02f1739b23c70493c0e4e6}





### `public inline  ~semaphore()` {#classgs_1_1semaphore_1adf5f8f601c377f8ecd6e1ab18f2d74a2}






# class `tlm_quantumkeeper_extended` {#classgs_1_1tlm__quantumkeeper__extended}

```
class tlm_quantumkeeper_extended
  : public tlm_utils::tlm_quantumkeeper
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  tlm_quantumkeeper_extended()` | 
`public inline virtual sc_core::sc_time time_to_sync()` | 
`public inline virtual void stop()` | 
`public inline virtual void start(std::function< void()> job)` | 
`public inline virtual SyncPolicy::Type get_thread_type() const` | 
`public inline virtual bool need_sync()` | 
`public inline virtual bool need_sync() const` | 
`public inline virtual void run_on_systemc(std::function< void()> job)` | 

## Members

### `public inline  tlm_quantumkeeper_extended()` {#classgs_1_1tlm__quantumkeeper__extended_1a85c439e93e7690784df6e6dd45deb979}





### `public inline virtual sc_core::sc_time time_to_sync()` {#classgs_1_1tlm__quantumkeeper__extended_1a12f0ef8b94a966789aacbb4cebbe630e}





### `public inline virtual void stop()` {#classgs_1_1tlm__quantumkeeper__extended_1a845d6bddccf78fe714be133ffedb2890}





### `public inline virtual void start(std::function< void()> job)` {#classgs_1_1tlm__quantumkeeper__extended_1aefbac7fbecd8afb4093f637e16995346}





### `public inline virtual SyncPolicy::Type get_thread_type() const` {#classgs_1_1tlm__quantumkeeper__extended_1a77487582d1094ff5373b510e8f19c473}





### `public inline virtual bool need_sync()` {#classgs_1_1tlm__quantumkeeper__extended_1a8f997816e68e43420d3f3fed9986f416}





### `public inline virtual bool need_sync() const` {#classgs_1_1tlm__quantumkeeper__extended_1a0c21807461dc0e9436bc72ef2590d8c5}





### `public inline virtual void run_on_systemc(std::function< void()> job)` {#classgs_1_1tlm__quantumkeeper__extended_1a1d48409e77ee45a34e6c805f69bc95e3}






# class `tlm_quantumkeeper_multi_quantum` {#classgs_1_1tlm__quantumkeeper__multi__quantum}

```
class tlm_quantumkeeper_multi_quantum
  : public gs::tlm_quantumkeeper_multithread
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

## Members


# class `tlm_quantumkeeper_multi_rolling` {#classgs_1_1tlm__quantumkeeper__multi__rolling}

```
class tlm_quantumkeeper_multi_rolling
  : public gs::tlm_quantumkeeper_multi_quantum
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  tlm_quantumkeeper_multi_rolling()` | 

## Members

### `public inline  tlm_quantumkeeper_multi_rolling()` {#classgs_1_1tlm__quantumkeeper__multi__rolling_1a3af117160f4eb0daa8d478511d5a10e0}






# class `tlm_quantumkeeper_multithread` {#classgs_1_1tlm__quantumkeeper__multithread}

```
class tlm_quantumkeeper_multithread
  : public gs::tlm_quantumkeeper_extended
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public virtual  ~tlm_quantumkeeper_multithread()` | 
`public  tlm_quantumkeeper_multithread()` | 
`public inline virtual SyncPolicy::Type get_thread_type() const` | 
`public virtual void start(std::function< void()> job)` | 
`public virtual void stop()` | 
`public virtual sc_core::sc_time time_to_sync()` | 
`public void inc(const sc_core::sc_time & t)` | 
`public void set(const sc_core::sc_time & t)` | 
`public virtual void sync()` | 
`public void reset()` | 
`public sc_core::sc_time get_current_time() const` | 
`public sc_core::sc_time get_local_time() const` | 
`public virtual bool need_sync()` | 
`protected enum gs::tlm_quantumkeeper_multithread::jobstates status` | 
`protected `[`async_event`](#classgs_1_1async__event)` m_tick` | 
`protected virtual bool is_sysc_thread() const` | 

## Members

### `public virtual  ~tlm_quantumkeeper_multithread()` {#classgs_1_1tlm__quantumkeeper__multithread_1a0a1a8bf3da4276d7759ff165b50f270f}





### `public  tlm_quantumkeeper_multithread()` {#classgs_1_1tlm__quantumkeeper__multithread_1aad083a4c46148fdb5a1dd96010c40b2b}





### `public inline virtual SyncPolicy::Type get_thread_type() const` {#classgs_1_1tlm__quantumkeeper__multithread_1a596e8a7252ce6d708d78fb5e4e404586}





### `public virtual void start(std::function< void()> job)` {#classgs_1_1tlm__quantumkeeper__multithread_1a3aa76a419d80e06808d5de23f79a391b}





### `public virtual void stop()` {#classgs_1_1tlm__quantumkeeper__multithread_1a308a5a44844309cb172f1f58345e77c6}





### `public virtual sc_core::sc_time time_to_sync()` {#classgs_1_1tlm__quantumkeeper__multithread_1aaa244ee9b0f3151e8d8f9190266e1542}





### `public void inc(const sc_core::sc_time & t)` {#classgs_1_1tlm__quantumkeeper__multithread_1a05b7b5b6a5c27e3676993dba7ab5b8fe}





### `public void set(const sc_core::sc_time & t)` {#classgs_1_1tlm__quantumkeeper__multithread_1ae6cc0be7d9c6ec80f9ce48e37a89076e}





### `public virtual void sync()` {#classgs_1_1tlm__quantumkeeper__multithread_1a4bdf4c4eea68a49d1747f7c9c09faeda}





### `public void reset()` {#classgs_1_1tlm__quantumkeeper__multithread_1aa58ec0bed90a0b45c82f179321921f41}





### `public sc_core::sc_time get_current_time() const` {#classgs_1_1tlm__quantumkeeper__multithread_1aadefd8aa6b7e200e2f690f3053c6c373}





### `public sc_core::sc_time get_local_time() const` {#classgs_1_1tlm__quantumkeeper__multithread_1ab9095db1ba59b85bb7f81cbf2f0b9f4d}





### `public virtual bool need_sync()` {#classgs_1_1tlm__quantumkeeper__multithread_1acff972051ac3babd2df36973c5b8aedd}





### `protected enum gs::tlm_quantumkeeper_multithread::jobstates status` {#classgs_1_1tlm__quantumkeeper__multithread_1ab2a7a3d09bb8dbbf2f203b0df7d5861b}





### `protected `[`async_event`](#classgs_1_1async__event)` m_tick` {#classgs_1_1tlm__quantumkeeper__multithread_1ac01e274b973335130b9d306729d80ca9}





### `protected virtual bool is_sysc_thread() const` {#classgs_1_1tlm__quantumkeeper__multithread_1ac4b2e17d1aa05acce14c9e69d49984a5}






# namespace `sc_core`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# namespace `tlm_utils`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

