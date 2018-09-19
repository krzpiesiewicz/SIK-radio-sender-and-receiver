#ifndef SIKSENDER_MAIN_THREAD_H
#define SIKSENDER_MAIN_THREAD_H

#include "smart_thread.h"
#include "time_alarm.h"
#include "siksender_shared_resource.h"
#include "siksender_control_port_service.h"
#include "siksender_audio_mainsender.h"
#include "siksender_rexmiter.h"

class MainThread : public ServerThread {
  shared_ptr<ControlPortService> control_port_service;
  shared_ptr<MainAudioSender> main_audio_sender;
  shared_ptr<Rexmiter> rexmiter;
public:
  MainThread(shared_ptr<SharedResource> & resource_) : ServerThread("main",
                                                             resource_) {

    add_release_function([this] {
      this->release();
    });

    control_port_service = SmartThread::create<ControlPortService>(resource);
    main_audio_sender = SmartThread::create<MainAudioSender>(resource);
    rexmiter = SmartThread::create<Rexmiter>(resource);
  }

  void release() {
    SmartThread::terminate(control_port_service);
    SmartThread::terminate(main_audio_sender);
    SmartThread::terminate(rexmiter);
  }

  void run() override {

    control_port_service->start();
    main_audio_sender->start();
    rexmiter->start();

    SmartThread::join(main_audio_sender);
  }
};

#endif // SIKSENDER_MAIN_THREAD_H
