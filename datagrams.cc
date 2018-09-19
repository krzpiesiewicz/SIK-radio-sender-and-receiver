#include <memory>
#include <cstddef>
#include <list>
#include <sstream>
#include <mutex>
#include "udp_sock_and_msg.h"
#include "datagrams.h"
#include "err.h"
#include "host_and_net.h"
#include "debug.h"

using namespace std;

size_t AudioDatagram::AUDIO_DATA_SIZE = 0;

size_t AudioDatagram::audio_datagram_all_size() {
  return AudioDatagram::HEADER_DATA_SIZE + AudioDatagram::AUDIO_DATA_SIZE;
}

AudioDatagram::AudioDatagram(Msg &msg) : data(msg.data) {

  if (msg.data_len != audio_datagram_all_size())
    throw ServerException("Data size from msg is not equal audio_datagram_all_size()");

//  byte *buff = read_header(session_id, first_byte_num, msg.data, msg.data_len);
//
//  size_t size = msg.data_len - (buff - msg.data);
//  audio_data = new byte[size];
//  read_audio_data(buff, audio_data, size);
}

const byte* AudioDatagram::read_header(uint64_t &session_id,
                                  uint64_t &first_byte_num,
                                  const byte *src_data, size_t data_size) {
  if (data_size < AudioDatagram::HEADER_DATA_SIZE)
    throw ServerException("The size of the header is too small");
  const byte *buff = src_data;
  session_id = read_ll(&buff);
  first_byte_num = read_ll(&buff);
  return buff;
}

const byte* AudioDatagram::read_audio_data(const byte* src_audio_data,
                                            byte* target_audio_data,
                                            size_t data_size) {
  if (data_size != AudioDatagram::AUDIO_DATA_SIZE)
    throw ServerException("The size of src_audio_data is not equal "
                          "AudioDatagram::AUDIO_DATA_SIZE");
  read_bytes(&src_audio_data, target_audio_data,
             AudioDatagram::AUDIO_DATA_SIZE);
  return src_audio_data;
}

Msg AudioDatagram::to_msg() {
//  size_t size = audio_datagram_all_size();
//  byte *data = new byte[size];
//  byte *buff = data;
//
//  write_ll(&buff, session_id);
//  write_ll(&buff, first_byte_num);
//  write_bytes(&buff, audio_data, AudioDatagram::AUDIO_DATA_SIZE);
//  return Msg{data, size};
  return Msg{data, audio_datagram_all_size()};
}

ControlDatagram::ControlDatagram(Msg &msg) {
  stringstream ss;
  string str{((char *) msg.get_data())};

  ss.str(str.substr(0, msg.data_len));
  string field;
  while (ss >> field) {
    fields.push_back(field);
  }
}

Msg ControlDatagram::to_msg() {
  stringstream ss{};
  for (auto it = fields.begin(); it != fields.end();) {
    ss << *it;
    if (++it != fields.end())
      ss << " ";
  }
  ss << "\n";

  string str = ss.str();
  shared_ptr<byte[]> data{new byte[str.length()], default_delete<byte[]>()};
  for (int i = 0; i < (int) str.length(); i++)
    data.get()[i] = (byte) str[i];

  return Msg{data, str.length()};
}

int ctrl_dt_genres_count = 3;

ControlDatagramEnum ctrl_dt_genre(ControlDatagram const &dt) {
  if (dt.fields.empty())
    throw ServerException("get_genre: ControlDatagram dt couldn't be "
                          "empty.");
  std::string const &first = (*dt.fields.begin());

  ControlDatagramEnum res = (ControlDatagramEnum) -1;
  int i = 0;
  while(i < ctrl_dt_genres_count && (int) res == -1) {
    if (first == ctrl_dt_first_field[i])
      res = (ControlDatagramEnum) i;
    else
      i++;
  }
  return res;
}

ControlDatagram lookup_datagram() {
  ControlDatagram dt;
  dt.add_field(ctrl_dt_first_field[(int) ControlDatagramEnum::LOOKUP]);
  return dt;
}

ControlDatagram replay_datagram(BasicSender const &sender) {
  ControlDatagram dt;
  dt.add_field(ctrl_dt_first_field[(int) ControlDatagramEnum::REPLAY]);
  dt.add_field(sender.mcast_addr);
  dt.add_field(to_string(sender.data_port));
  dt.add_field(sender.name);
  return dt;
}

//ControlDatagram rexmit_datagram(std::list<uint64_t> const &datagrams_numbers) {
//  ControlDatagram dt;
//  dt.add_field(ctrl_dt_first_field[(int) ControlDatagramEnum::REXMIT]);
//  stringstream ss;
//  for (auto it = datagrams_numbers.begin();;) {
//    ss << *it;
//    it++;
//    if (it != datagrams_numbers.end())
//      ss << ",";
//    else
//      break;
//  }
//  dt.add_field(ss.str());
//  return dt;
//}