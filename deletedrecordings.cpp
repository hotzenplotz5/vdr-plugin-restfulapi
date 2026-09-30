#include "deletedrecordings.h"

using namespace std;

namespace {

bool hasDeletedRecordingExtension(const string& path)
{
  return path.length() >= 4 && path.compare(path.length() - 4, 4, ".del") == 0;
}

}

void DeletedRecordingsResponder::reply(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler::addHeader(reply);

  if (request.method() == "OPTIONS") {
    reply.addHeader("Allow", "GET, POST");
    reply.httpReturn(200, "OK");
    return;
  }

  if ((int)request.url().find("/recordings/deleted/restore") == 0) {
    if (request.method() == "POST") {
      restoreDeletedRecording(out, request, reply);
    } else {
      reply.httpReturn(501, "Only POST method is supported by the /recordings/deleted/restore service.");
    }
    return;
  }

  if ((int)request.url().find("/recordings/deleted") == 0) {
    if (request.method() == "GET") {
      showDeletedRecordings(out, request, reply);
    } else {
      reply.httpReturn(501, "Only GET method is supported by the /recordings/deleted service.");
    }
    return;
  }

  reply.httpReturn(403, "Service not found");
}

void DeletedRecordingsResponder::showDeletedRecordings(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/deleted", request);

  if (!q.isFormat(".json")) {
    reply.httpReturn(502, "Resources are only available as .json for the /recordings/deleted service.");
    return;
  }

#if VDRVERSNUM >= 20708
  bool read_marks = q.getOptionAsString("marks") == "true";
  int start_filter = q.getOptionAsInt("start");
  int limit_filter = q.getOptionAsInt("limit");

  reply.addHeader("Content-Type", CT_JSON);

  RecordingList* recordingList = (RecordingList*)new JsonRecordingList(&out, read_marks);
  if (start_filter >= 0 && limit_filter >= 1) {
    recordingList->activateLimit(start_filter, limit_filter);
  }

  recordingList->init();

  LOCK_DELETEDRECORDINGS_READ;
  const cRecordings& deletedRecordings = *DeletedRecordings;

  for (int i = 0; i < deletedRecordings.Count(); i++) {
    recordingList->addRecording(deletedRecordings.Get(i), i, NULL, "");
  }

  recordingList->setTotal(deletedRecordings.Count());
  recordingList->finish();
  delete recordingList;
#else
  reply.httpReturn(501, "Deleted recordings require VDR 2.7.8 or newer.");
#endif
}

void DeletedRecordingsResponder::restoreDeletedRecording(ostream& out, cxxtools::http::Request& request, cxxtools::http::Reply& reply)
{
  QueryHandler q("/recordings/deleted/restore", request);
  string recordingFile = q.getBodyAsString("file");

  reply.addHeader("Content-Type", "text/plain; charset=utf-8");

  if (recordingFile.empty()) {
    reply.httpReturn(400, "Missing file name!");
    return;
  }

  if (!hasDeletedRecordingExtension(recordingFile)) {
    reply.httpReturn(400, "Deleted recording path must end with .del!");
    return;
  }

#if VDRVERSNUM >= 20708
  LOCK_RECORDINGS_WRITE;
  LOCK_DELETEDRECORDINGS_WRITE;

  cRecordings& recordings = *Recordings;
  cRecordings& deletedRecordings = *DeletedRecordings;
  cRecording* recording = deletedRecordings.GetByName(recordingFile.c_str());

  if (recording == NULL) {
    reply.httpReturn(404, "Deleted recording not found!");
    return;
  }

  esyslog("restfulapi: restore deleted recording %s", recording->FileName());

  if (!recording->Undelete()) {
    reply.httpReturn(409, "Recording could not be restored!");
    return;
  }

  deletedRecordings.Del(recording, false);
  recordings.Add(recording);
  cVideoDiskUsage::ForceCheck();

  reply.httpReturn(200, "Recording restored!");
#else
  reply.httpReturn(501, "Restoring deleted recordings requires VDR 2.7.8 or newer.");
#endif
}
