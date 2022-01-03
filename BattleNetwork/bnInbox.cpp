#include "bnInbox.h"

void Inbox::AddMail(const Inbox::Mail& mail)
{
  mailList.push_back(mail);
}

void Inbox::RemoveMail(size_t index)
{
  if (index >= mailList.size()) return;

  mailList.erase(mailList.begin() + index);
}

void Inbox::ReadMail(size_t index, std::function<void(const Inbox::Mail& msg)> onRead)
{
  if (index >= mailList.size() || !onRead) return;

  auto& mail = mailList.at(index);
  mail.read = true;
  onRead(mail);
}

const Inbox::Mail& Inbox::GetAt(size_t index) const
{
  return mailList.at(index);
}

void Inbox::Clear()
{
  mailList.clear();
}

bool Inbox::Empty() const
{
  return Size() == 0;
}

size_t Inbox::Size() const
{
  return mailList.size();
}
