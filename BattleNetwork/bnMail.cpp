#include "bnMail.h"

void Mail::AddMessage(const Message& msg)
{
  inbox.push_back(msg);
}

void Mail::RemoveMessage(size_t index)
{
  if (index >= inbox.size()) return;

  inbox.erase(inbox.begin() + index);
}

void Mail::ReadMessage(size_t index, std::function<void(const Message& msg)> onRead)
{
  if (index >= inbox.size() || !onRead) return;

  auto& msg = inbox.at(index);
  msg.read = true;
  onRead(msg);
}

const Mail::Message& Mail::GetAt(size_t index) const
{
  return inbox.at(index);
}

void Mail::Clear()
{
  inbox.clear();
}

bool Mail::Empty() const
{
  return Size() == 0;
}

size_t Mail::Size() const
{
  return inbox.size();
}
