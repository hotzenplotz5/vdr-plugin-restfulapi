#ifndef __DELETEDRECORDINGS_H
#define __DELETEDRECORDINGS_H

#include "recordings.h"
#include <vdr/menu.h>

class DeletedRecordingsResponder : public cxxtools::http::Responder
{
private:
  const char* CT_JSON;

public:
  explicit DeletedRecordingsResponder(cxxtools::http::Service& service)
    : cxxtools::http::Responder(service)
  {
    CT_JSON = "application/json; charset=utf-8";
  }

  virtual void reply(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
  void showDeletedRecordings(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
  void restoreDeletedRecording(std::ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply);
};

typedef cxxtools::http::CachedService<DeletedRecordingsResponder> DeletedRecordingsService;

#endif
