#include "messages.h"
#include <SPI.h>

Messages_ &Messages_::getInstance()
{
  static Messages_ instance;
  return instance;
}

void Messages_::add(std::string text, int repeat, int id, int delay,
                    std::vector<int> graph, int miny, int maxy)
{
  remove(id);

  Message *msg = messagePool.acquire();
  if (msg)
  {
      msg->id = id;
      msg->pendingRemove = false;
      msg->repeat = repeat;
      msg->delay = delay;
      msg->text = text;
      msg->graph = graph;
      msg->miny = miny;
      msg->maxy = maxy;

      activeMessages.push_back(msg);
      previousMinute = -1;
      _wasScrolling = true;
  }
  else
  {
    Serial.println("Warning: Message pool exhausted!");
  }
}

void Messages_::remove(int id)
{
    auto it = std::find_if(activeMessages.begin(), activeMessages.end(),
                           [id](const Message *msg)
                           { return msg->id == id; });

    if (it != activeMessages.end())
    {
        (*it)->pendingRemove = true;
    }
}

void Messages_::scroll()
{
  for (auto it = activeMessages.begin(); it != activeMessages.end();)
  {
    Message *msg = *it;

    // Run all repeats in one blocking call so the main loop
    // doesn't interfere between passes
    int passes = (msg->repeat == -1) ? 1 : msg->repeat;
    bool infinite = (msg->repeat == -1);

    if (infinite)
    {
        if (msg->text.length() > 0)
            Screen.scrollText(msg->text.c_str(), msg->delay);
        if (msg->graph.size() > 0)
            Screen.scrollGraph(msg->graph, msg->miny, msg->maxy, msg->delay);

        // Check flag after each pass
        if (msg->pendingRemove)
        {
            messagePool.release(msg);
            it = activeMessages.erase(it);
        }
        else
        {
            ++it;
        }
    }
    else
    {
      // Finite — run ALL passes back to back right here
      for (int i = 0; i < passes; i++)
      {
        if (msg->text.length() > 0)
          Screen.scrollText(msg->text.c_str(), msg->delay);
        if (msg->graph.size() > 0)
          Screen.scrollGraph(msg->graph, msg->miny, msg->maxy, msg->delay);
      }
      // Done all passes, remove message
      messagePool.release(msg);
      it = activeMessages.erase(it);
    }
  }
}

void Messages_::scrollMessageEveryMinute()
{
  struct tm timeinfo;

  if (getLocalTime(&timeinfo))
  {
    if (timeinfo.tm_min != previousMinute)
    {
      scroll();
      previousMinute = timeinfo.tm_min;
    }

    if (timeinfo.tm_sec != previousSecond)
    {
      if (!activeMessages.empty())
      {
        indicatorPixel = timeinfo.tm_sec & 0b00000001;
        Screen.setPixel(0, 0, indicatorPixel);
      }
      else if (indicatorPixel > 0)
      {
        indicatorPixel = 0;
        Screen.setPixel(0, 0, indicatorPixel);
      }
      previousSecond = timeinfo.tm_sec;
    }
  }
}

Messages_ &Messages = Messages.getInstance();