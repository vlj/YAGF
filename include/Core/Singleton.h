// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#ifndef SINGLETON_H
#define SINGLETON_H

template <typename Type>
class Singleton
{
private:
  static Type *Instance;
public:
  static Type *getInstance()
  {
    if (Instance == nullptr)
      Instance = new Type();
    return Instance;
  }

  static void kill()
  {
    if (Instance != nullptr)
      delete Instance;
  }
};

template <typename Type> Type *Singleton<Type>::Instance = nullptr;

#endif
