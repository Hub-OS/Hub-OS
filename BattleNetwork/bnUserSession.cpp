#include "bnUserSession.h"

UserSession * UserSession::LoadUserSession(std::string path)
{
  return nullptr;
}

void UserSession::WriteUserSession(std::string path)
{

}

void UserSession::CloseUserSession(UserSession * session)
{
  if (session) delete session;
}
