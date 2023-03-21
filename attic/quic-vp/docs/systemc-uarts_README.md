# namespace `sc_core`



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------

# class `CharBackend` {#class_char_backend}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public inline  CharBackend()` | 
`public void write(unsigned char c)` | 
`public inline void register_receive(void * opaque,void(*)(void *opaque, const uint8_t *buf, int size) receive,int(*)(void *opaque) can_receive)` | 
`protected void * m_opaque` | 
`protected void(* m_receive` | 
`protected int(* m_can_receive` | 

## Members

### `public inline  CharBackend()` {#class_char_backend_1a5635bd47ad5575bb29f7a6561280e77d}





### `public void write(unsigned char c)` {#class_char_backend_1a671d349a8e08e2a655d7e95bf71fac5a}





### `public inline void register_receive(void * opaque,void(*)(void *opaque, const uint8_t *buf, int size) receive,int(*)(void *opaque) can_receive)` {#class_char_backend_1a4b007687d7e2261fd527b3af8e4dcaf5}





### `protected void * m_opaque` {#class_char_backend_1ad3d9036215ae0c64c0314d097794ae70}





### `protected void(* m_receive` {#class_char_backend_1a3e0a8ad612489a8257efff90378a4f23}





### `protected int(* m_can_receive` {#class_char_backend_1ab03bfc0301bc9b53b789b6a0721a540e}






# class `CharBackendSocket` {#class_char_backend_socket}

```
class CharBackendSocket
  : public CharBackend
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public std::string type` | 
`public std::string address` | 
`public bool m_server` | 
`public int m_srv_socket` | 
`public bool m_nowait` | 
`public int m_socket` | 
`public uint8_t m_buf` | 
`public  SC_HAS_PROCESS(`[`CharBackendSocket`](#class_char_backend_socket)`)` | 
`public inline void start_of_simulation()` | 
`public inline  CharBackendSocket(sc_core::sc_module_name name,std::string type,std::string address,bool server,bool nowait)` | 
`public inline void * rcv_thread()` | 
`public inline void rcv(void)` | 
`public inline virtual void write(unsigned char c)` | 
`public inline void setup_tcp_server(std::string ip,std::string port)` | 
`public inline void setup_tcp_client(std::string ip,std::string port)` | 

## Members

### `public std::string type` {#class_char_backend_socket_1a5497f2f72ee0d85020510098dd50efff}





### `public std::string address` {#class_char_backend_socket_1a9702e03a68feefdc881e9401bed9f5b6}





### `public bool m_server` {#class_char_backend_socket_1a03ac0ead37249fda0f310475c025b4c9}





### `public int m_srv_socket` {#class_char_backend_socket_1af6d493f01b823905573630ddc2bb0e18}





### `public bool m_nowait` {#class_char_backend_socket_1a4c5d13510e14a827d7175474a9b101b5}





### `public int m_socket` {#class_char_backend_socket_1a65f94ad56e3640d9091d4d3bd90aa606}





### `public uint8_t m_buf` {#class_char_backend_socket_1ab6ef9b6a7c3c8e68fdfdaf14819b27aa}





### `public  SC_HAS_PROCESS(`[`CharBackendSocket`](#class_char_backend_socket)`)` {#class_char_backend_socket_1a7d7f40b906297e55b3d897aaad39abe4}





### `public inline void start_of_simulation()` {#class_char_backend_socket_1ae922741d53d6f6a1d7c8959bb19364ac}





### `public inline  CharBackendSocket(sc_core::sc_module_name name,std::string type,std::string address,bool server,bool nowait)` {#class_char_backend_socket_1a7849d3f07da653f093873e0e58c7db72}





### `public inline void * rcv_thread()` {#class_char_backend_socket_1a5b04f176297e3ad478e93ee23af9c0ce}





### `public inline void rcv(void)` {#class_char_backend_socket_1aa23a3c6573c84d1c31bfb1dd9647789c}





### `public inline virtual void write(unsigned char c)` {#class_char_backend_socket_1ac5ddd3c42660d616df2a4bf9bbe8aa88}





### `public inline void setup_tcp_server(std::string ip,std::string port)` {#class_char_backend_socket_1aa632bfc4aaded4cb32302a90433476a7}





### `public inline void setup_tcp_client(std::string ip,std::string port)` {#class_char_backend_socket_1a7e68b685fd3eee74198ba5f5c8f796dd}






# class `CharBackendStdio` {#class_char_backend_stdio}

```
class CharBackendStdio
  : public CharBackend
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  SC_HAS_PROCESS(`[`CharBackendStdio`](#class_char_backend_stdio)`)` | 
`public inline  CharBackendStdio(sc_core::sc_module_name name)` | 
`public inline void * rcv_thread()` | 
`public inline void rcv(void)` | 
`public inline virtual void write(unsigned char c)` | 

## Members

### `public  SC_HAS_PROCESS(`[`CharBackendStdio`](#class_char_backend_stdio)`)` {#class_char_backend_stdio_1a6dca38b85ec3377a0f8bdda3406835a7}





### `public inline  CharBackendStdio(sc_core::sc_module_name name)` {#class_char_backend_stdio_1ae4c77b4e22cf0a5347fcb8b0a84d585f}





### `public inline void * rcv_thread()` {#class_char_backend_stdio_1a1b4e3ceedb8d459bf8bdfaa953e0eaa5}





### `public inline void rcv(void)` {#class_char_backend_stdio_1a498a15d517c5ce7ac0133256ce77c625}





### `public inline virtual void write(unsigned char c)` {#class_char_backend_stdio_1a511f8c833656d6d0fe34b38c5ae5ff0b}






# class `IbexUart` {#class_ibex_uart}

```
class IbexUart
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public sc_core::sc_out< bool > irq_rxwatermark` | 
`public uint32_t it_state` | 
`public uint32_t it_enable` | 
`public uint32_t ctrl` | 
`public uint32_t status` | 
`public uint8_t rdata` | 
`public `[`CharBackend`](#class_char_backend)` * chr` | 
`public tlm_utils::simple_target_socket< `[`IbexUart`](#class_ibex_uart)` > socket` | 
`public sc_core::sc_event update_event` | 
`public inline void update_irqs(void)` | 
`public inline void recv(const uint8_t * buf,int size)` | 
`public  SC_HAS_PROCESS(`[`IbexUart`](#class_ibex_uart)`)` | 
`public inline  IbexUart(sc_core::sc_module_name name)` | 
`public inline void set_backend(`[`CharBackend`](#class_char_backend)` * backend)` | 
`public inline void b_transport(tlm::tlm_generic_payload & trans,sc_core::sc_time & delay)` | 
`public inline uint64_t reg_read(uint64_t addr)` | 
`public inline void reg_write(uint64_t addr,uint32_t data)` | 
`public inline void before_end_of_elaboration()` | 

## Members

### `public sc_core::sc_out< bool > irq_rxwatermark` {#class_ibex_uart_1ae8ee3d000e791bff010747761541b9fa}





### `public uint32_t it_state` {#class_ibex_uart_1afe768ce6758ef9cbfb49cfa6a9712ec9}





### `public uint32_t it_enable` {#class_ibex_uart_1afc30833d31516260077386666cbab643}





### `public uint32_t ctrl` {#class_ibex_uart_1a3821767c09c26a1cb6f0ecf3bff3fde1}





### `public uint32_t status` {#class_ibex_uart_1a3f11e2736842ed3ef983e95401dc30c0}





### `public uint8_t rdata` {#class_ibex_uart_1ac7d21a6b85816aea727f03b2681b9bfd}





### `public `[`CharBackend`](#class_char_backend)` * chr` {#class_ibex_uart_1ae45fc1d20d46a30967725ebe8c3bb316}





### `public tlm_utils::simple_target_socket< `[`IbexUart`](#class_ibex_uart)` > socket` {#class_ibex_uart_1a4a057ab84a9ab50122a9a29361a61b8e}





### `public sc_core::sc_event update_event` {#class_ibex_uart_1ace4fc4314c156074d4c80ed9c9fdafdb}





### `public inline void update_irqs(void)` {#class_ibex_uart_1ad58ccb8eb87f4d4e0cd1ac195f4880d6}





### `public inline void recv(const uint8_t * buf,int size)` {#class_ibex_uart_1ad1a9aca02d40a5a436d43a23e291a21e}





### `public  SC_HAS_PROCESS(`[`IbexUart`](#class_ibex_uart)`)` {#class_ibex_uart_1a30385ea366ed567570cd947ea50badd5}





### `public inline  IbexUart(sc_core::sc_module_name name)` {#class_ibex_uart_1a13fa0d1f12cd6b9fe477206bca6e128e}





### `public inline void set_backend(`[`CharBackend`](#class_char_backend)` * backend)` {#class_ibex_uart_1a42557d9b705c232dcf3474cafb918a77}





### `public inline void b_transport(tlm::tlm_generic_payload & trans,sc_core::sc_time & delay)` {#class_ibex_uart_1a4182d06f590aa48fcd386a66f37c5bab}





### `public inline uint64_t reg_read(uint64_t addr)` {#class_ibex_uart_1a8029421fe575f62f7209c113265ec735}





### `public inline void reg_write(uint64_t addr,uint32_t data)` {#class_ibex_uart_1a674b23ea940b4aeb4d9c9f0a5a32665f}





### `public inline void before_end_of_elaboration()` {#class_ibex_uart_1ada4515d52b373b56883223372df6e0d1}






# class `Pl011` {#class_pl011}

```
class Pl011
  : public sc_core::sc_module
```  





## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public `[`PL011State`](#struct_p_l011_state)` * s` | 
`public `[`CharBackend`](#class_char_backend)` * chr` | 
`public tlm_utils::simple_target_socket< `[`Pl011`](#class_pl011)` > socket` | 
`public sc_core::sc_vector< sc_core::sc_signal< bool, sc_core::SC_MANY_WRITERS > > irq` | 
`public sc_core::sc_event update_event` | 
`public  SC_HAS_PROCESS(`[`Pl011`](#class_pl011)`)` | 
`public inline  Pl011(sc_core::sc_module_name name)` | 
`public inline void set_backend(`[`CharBackend`](#class_char_backend)` * backend)` | 
`public inline void write(uint64_t offset,uint64_t value)` | 
`public inline void b_transport(tlm::tlm_generic_payload & trans,sc_core::sc_time & delay)` | 
`public inline void pl011_update()` | 
`public inline void pl011_update_sysc()` | 
`public inline uint64_t pl011_read(uint64_t offset)` | 
`public inline void pl011_set_read_trigger()` | 
`public inline void pl011_write(uint64_t offset,uint64_t value)` | 
`public inline void pl011_put_fifo(uint32_t value)` | 

## Members

### `public `[`PL011State`](#struct_p_l011_state)` * s` {#class_pl011_1aaa20f80d5875c0bed94c2b03f55d146c}





### `public `[`CharBackend`](#class_char_backend)` * chr` {#class_pl011_1a601b6627e9ff8717b8a7b3763f2614fe}





### `public tlm_utils::simple_target_socket< `[`Pl011`](#class_pl011)` > socket` {#class_pl011_1a69f65ac9e7ce61c172430925fc588ed1}





### `public sc_core::sc_vector< sc_core::sc_signal< bool, sc_core::SC_MANY_WRITERS > > irq` {#class_pl011_1a42474e161615ef0dfa5ab3f0902f3bea}





### `public sc_core::sc_event update_event` {#class_pl011_1af7e6ad1bc0708b5702b02c517228a738}





### `public  SC_HAS_PROCESS(`[`Pl011`](#class_pl011)`)` {#class_pl011_1a7518426ecc00f42699b55ae330611e47}





### `public inline  Pl011(sc_core::sc_module_name name)` {#class_pl011_1a0a894478ff308936d4fa7469fb3e40c3}





### `public inline void set_backend(`[`CharBackend`](#class_char_backend)` * backend)` {#class_pl011_1a7cecd4a049bcaa90b511128ad3a36192}





### `public inline void write(uint64_t offset,uint64_t value)` {#class_pl011_1a29a9a72cf21f6d9ef83f82834c3d6173}





### `public inline void b_transport(tlm::tlm_generic_payload & trans,sc_core::sc_time & delay)` {#class_pl011_1a77fcfb53418772cb03661f11acf8a662}





### `public inline void pl011_update()` {#class_pl011_1a46b24ddf0b7e8b2f6eb6083967133149}





### `public inline void pl011_update_sysc()` {#class_pl011_1a44a5705622c01061a0b3e0982a4821e9}





### `public inline uint64_t pl011_read(uint64_t offset)` {#class_pl011_1a86b39bd4f1875a7bbbb13797aca0a491}





### `public inline void pl011_set_read_trigger()` {#class_pl011_1aebf36f12d2c846f202e3137e31d15e0e}





### `public inline void pl011_write(uint64_t offset,uint64_t value)` {#class_pl011_1a041d84343f1bd32322163f810e15f443}





### `public inline void pl011_put_fifo(uint32_t value)` {#class_pl011_1ac4d44374a71aa756a640b90976a27ba2}






# struct `PL011State` {#struct_p_l011_state}






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public uint32_t readbuff` | 
`public uint32_t flags` | 
`public uint32_t lcr` | 
`public uint32_t rsr` | 
`public uint32_t cr` | 
`public uint32_t dmacr` | 
`public uint32_t int_enabled` | 
`public uint32_t int_level` | 
`public uint32_t read_fifo` | 
`public uint32_t ilpr` | 
`public uint32_t ibrd` | 
`public uint32_t fbrd` | 
`public uint32_t ifl` | 
`public int read_pos` | 
`public int read_count` | 
`public int read_trigger` | 
`public const unsigned char * id` | 

## Members

### `public uint32_t readbuff` {#struct_p_l011_state_1a1680d284da461ef68becf6be09d1272d}





### `public uint32_t flags` {#struct_p_l011_state_1a06d1c8f2171ee45246bd2064b90cf829}





### `public uint32_t lcr` {#struct_p_l011_state_1ac43db5779da66876a166f323bdbc281c}





### `public uint32_t rsr` {#struct_p_l011_state_1a54404e594eeb2b1dd4c9ca59a9b4e8f2}





### `public uint32_t cr` {#struct_p_l011_state_1aeb610da022a965671677d5a3cc11613a}





### `public uint32_t dmacr` {#struct_p_l011_state_1a522f6c5c5afe07f71c1487262ee45903}





### `public uint32_t int_enabled` {#struct_p_l011_state_1ac42f0ef89d66efe33b2860844fd421e1}





### `public uint32_t int_level` {#struct_p_l011_state_1a6081bea179c6fb28a55b58747df44726}





### `public uint32_t read_fifo` {#struct_p_l011_state_1acba1dd73b795e070084525e86bce990f}





### `public uint32_t ilpr` {#struct_p_l011_state_1a112e03cd2590c3fd28f9b50365acd67e}





### `public uint32_t ibrd` {#struct_p_l011_state_1ad6a2fd51719c2786931ef204fe832269}





### `public uint32_t fbrd` {#struct_p_l011_state_1aa21a98c80248a401301649d56e187890}





### `public uint32_t ifl` {#struct_p_l011_state_1a5c368f4c73ec69861658ed84206d9f3e}





### `public int read_pos` {#struct_p_l011_state_1a2105cffeedc57ad70a6330cea6ba2ab0}





### `public int read_count` {#struct_p_l011_state_1a51cfbc8476999804ebe3611cc81a06b4}





### `public int read_trigger` {#struct_p_l011_state_1a6d7b386e2c19f4c113a707a787da1c68}





### `public const unsigned char * id` {#struct_p_l011_state_1a6f098119cf6fae65a7f5adb82e1d10ba}






