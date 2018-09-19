#ifndef DATAGRAMS_H
#define DATAGRAMS_H

#include <memory>
#include <cstddef>
#include <list>
#include <set>
#include <mutex>
#include <string>
#include <sstream>

#include "udp_sock_and_msg.h"
#include "host_and_net.h"
#include "debug.h"

struct Datagram {
};

struct AudioDatagram : Datagram {

  static std::size_t AUDIO_DATA_SIZE;

  const static std::size_t HEADER_DATA_SIZE = 2 * sizeof(uint64_t);

  static std::size_t audio_datagram_all_size();

  std::shared_ptr<byte[]> data;

  inline uint64_t session_id() const {
    const std::byte* ptr = session_id_ptr();
    return read_ll(&ptr);
  }

  inline uint64_t first_byte_num() const {
    const std::byte* ptr = first_byte_num_ptr();
    return read_ll(&ptr);
  }

  inline const std::byte* audio_data() const {
    return data.get() + HEADER_DATA_SIZE;
  }

  inline const std::byte* session_id_ptr() const {
    return data.get();
  }

  inline const std::byte* first_byte_num_ptr() const {
    return data.get() + sizeof(uint64_t);
  }

  inline std::byte *audio_data() {
    return data.get() + HEADER_DATA_SIZE;
  }

  inline std::byte* session_id_ptr() {
    return data.get();
  }

  inline std::byte* first_byte_num_ptr() {
    return data.get() + sizeof(uint64_t);
  }

  inline void session_id(uint64_t new_val) {
    byte* buff = (byte*) session_id_ptr();
    write_ll(&buff, new_val);
  }

  inline void first_byte_num(uint64_t new_val) {
    byte* buff = (byte*) first_byte_num_ptr();
    write_ll(&buff, new_val);
  }

  AudioDatagram() : data(new std::byte[audio_datagram_all_size()],
                         std::default_delete<byte[]>()) {}

  AudioDatagram(uint64_t session_id_, uint64_t first_byte_num_) :
    data(new std::byte[audio_datagram_all_size()],
         std::default_delete<byte[]>()) {
    session_id(session_id_);
    first_byte_num(first_byte_num_);
  }

//  ~AudioDatagram() {
//    if (audio_data != nullptr) {
//      delete[] audio_data;
//      audio_data = nullptr;
//    }
//  }

  /**
   * Reads header from src_data buffer
   * @param session_id
   * @param first_byte_num
   * @param src_data
   * @param src_data_size
   * @return a pointer to the rest of src_data buffer
   */
  static const std::byte *read_header(uint64_t &session_id,
                                uint64_t &first_byte_num,
                                const std::byte *src_data,
                                size_t src_data_size);

  /**
   * Reads audio data from src_audio_data buffer
   * @param src_audio_data
   * @param target_audio_data
   * @return a pointer to the rest of src_data buffer
   */
  static const std::byte *read_audio_data(const std::byte *src_audio_data,
                                    std::byte *target_audio_data,
                                    std::size_t src_size);

  AudioDatagram(Msg &msg);

  Msg to_msg();
};

struct ControlDatagram : Datagram {

  std::list<std::string> fields;

  ControlDatagram() = default;

  ControlDatagram(Msg &msg);

  Msg to_msg();

  void add_field(std::string field) {fields.push_back(field);}
};

class BasicSender {
  std::string name, mcast_addr;
  int data_port;
public:
  BasicSender(std::string name_, std::string mcast_addr_, int data_port_) :
    name(name_), mcast_addr(mcast_addr_), data_port(data_port_) {}

  friend ControlDatagram replay_datagram(BasicSender const &sender);
};

enum class ControlDatagramEnum {
  LOOKUP = 0, REPLAY = 1, REXMIT = 2
};

static std::string ctrl_dt_first_field[] = {
  "ZERO_SEVEN_COME_IN", "BOREWICZ_HERE", "LOUDER_PLEASE"};

ControlDatagramEnum ctrl_dt_genre(ControlDatagram const &dt);

ControlDatagram lookup_datagram();

ControlDatagram replay_datagram(BasicSender const &sender);

template<class RandomIt>
ControlDatagram rexmit_datagram(RandomIt dt_num_first, RandomIt dt_num_last) {
  ControlDatagram dt;
  dt.add_field(ctrl_dt_first_field[(int) ControlDatagramEnum::REXMIT]);
  stringstream ss;
  for (auto it = dt_num_first;;) {
    ss << *it;
    it++;
    if (it != dt_num_last)
      ss << ",";
    else
      break;
  }
  auto str = ss.str();
  dt.add_field(str);
  return dt;
}

#endif // DATAGRAMS_H