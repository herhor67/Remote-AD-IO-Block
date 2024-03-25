/******************************************************************************
The MIT License(MIT)

Embedded Template Library.
https://github.com/ETLCPP/etl
https://www.etlcpp.com

Copyright(c) 2020 John Wellbelove

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

/*[[[cog
import cog
cog.outl("#if 0")
]]]*/
/*[[[end]]]*/
#error THIS HEADER IS A GENERATOR. DO NOT INCLUDE.
/*[[[cog
import cog
cog.outl("#endif")
]]]*/
/*[[[end]]]*/

/*[[[cog
import cog
cog.outl("//***************************************************************************")
cog.outl("// THIS FILE HAS BEEN AUTO GENERATED. DO NOT EDIT THIS FILE.")
cog.outl("//***************************************************************************")
]]]*/
/*[[[end]]]*/

//***************************************************************************
// To generate to header file, run this at the command line.
// Note: You will need Python and COG installed.
//
// python -m cogapp -d -e -omessage_packet.h -DHandlers=<n> message_packet_generator.h
// Where <n> is the number of messages to support.
//
// e.g.
// To generate handlers for up to 16 messages...
// python -m cogapp -d -e -omessage_packet.h -DHandlers=16 message_packet_generator.h
//
// See generate.bat
//***************************************************************************

#ifndef ETL_MESSAGE_PACKET_INCLUDED
#define ETL_MESSAGE_PACKET_INCLUDED

#include "platform.h"

#if ETL_HAS_VIRTUAL_MESSAGES

#include "message.h"
#include "error_handler.h"
#include "static_assert.h"
#include "largest.h"
#include "alignment.h"
#include "utility.h"

#include <stdint.h>

namespace etl
{
#if ETL_USING_CPP17 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)
  //***************************************************************************
  // The definition for all message types.
  //***************************************************************************
  template <typename... TMessageTypes>
  class message_packet
  {

  private:

    template <typename T>
    static constexpr bool IsMessagePacket = etl::is_same_v< etl::remove_const_t<etl::remove_reference_t<T>>, etl::message_packet<TMessageTypes...>>;

    template <typename T>
    static constexpr bool IsInMessageList = etl::is_one_of_v<etl::remove_const_t<etl::remove_reference_t<T>>, TMessageTypes...>;

    template <typename T>
    static constexpr bool IsIMessage = etl::is_same_v<remove_const_t<etl::remove_reference_t<T>>, etl::imessage>;

  public:

    //********************************************
#include "private/diagnostic_uninitialized_push.h"
    message_packet()
      : valid(false)
    {
    }
#include "private/diagnostic_pop.h"

    //********************************************
    ///
    //********************************************
#include "private/diagnostic_uninitialized_push.h"
    template <typename T>
    explicit message_packet(T&& msg)
      : valid(true)
    {
      if constexpr (IsIMessage<T>)
      {
        if (accepts(msg))
        {
          add_new_message(etl::forward<T>(msg));
          valid = true;
        }
        else
        {
          valid = false;
        }

        ETL_ASSERT(valid, ETL_ERROR(unhandled_message_exception));
      }
      else if constexpr (IsInMessageList<T>)
      {
        add_new_message_type<T>(etl::forward<T>(msg));
      }
      else if constexpr (IsMessagePacket<T>)
      {
        copy(etl::forward<T>(msg));
      }
      else
      {
        ETL_STATIC_ASSERT(IsInMessageList<T>, "Message not in packet type list");
      }
    }
#include "private/diagnostic_pop.h"

    //**********************************************
    void copy(const message_packet& other)
    {
      valid = other.is_valid();

      if (valid)
      {
        add_new_message(other.get());
      }
    }

    //**********************************************
    void copy(message_packet&& other)
    {
      valid = other.is_valid();

      if (valid)
      {
        add_new_message(etl::move(other.get()));
      }
    }

    //**********************************************
#include "private/diagnostic_uninitialized_push.h"
    message_packet& operator =(const message_packet& rhs)
    {
      delete_current_message();
      valid = rhs.is_valid();
      if (valid)
      {
        add_new_message(rhs.get());
      }

      return *this;
    }
#include "private/diagnostic_pop.h"

    //**********************************************
#include "private/diagnostic_uninitialized_push.h"
    message_packet& operator =(message_packet&& rhs)
    {
      delete_current_message();
      valid = rhs.is_valid();
      if (valid)
      {
        add_new_message(etl::move(rhs.get()));
      }

      return *this;
    }
#include "private/diagnostic_pop.h"

    //********************************************
    ~message_packet()
    {
      delete_current_message();
    }

    //********************************************
    etl::imessage& get() ETL_NOEXCEPT
    {
      return *static_cast<etl::imessage*>(data);
    }

    //********************************************
    const etl::imessage& get() const ETL_NOEXCEPT
    {
      return *static_cast<const etl::imessage*>(data);
    }

    //********************************************
    bool is_valid() const
    {
      return valid;
    }

    //**********************************************
    static ETL_CONSTEXPR bool accepts(etl::message_id_t id)
    {
      return (accepts_message<TMessageTypes::ID>(id) || ...);
    }

    //**********************************************
    static ETL_CONSTEXPR bool accepts(const etl::imessage& msg)
    {
      return accepts(msg.get_message_id());
    }

    //**********************************************
    template <etl::message_id_t Id>
    static ETL_CONSTEXPR bool accepts()
    {
      return (accepts_message<TMessageTypes::ID, Id>() || ...);
    }

    //**********************************************
    template <typename TMessage>
    static ETL_CONSTEXPR
      typename etl::enable_if<etl::is_base_of<etl::imessage, TMessage>::value, bool>::type
      accepts()
    {
      return accepts<TMessage::ID>();
    }

    enum
    {
      SIZE = etl::largest<TMessageTypes...>::size,
      ALIGNMENT = etl::largest<TMessageTypes...>::alignment
    };

  private:

    //**********************************************
    template <etl::message_id_t Id1, etl::message_id_t Id2>
    static bool accepts_message()
    {
      return Id1 == Id2;
    }

    //**********************************************
    template <etl::message_id_t Id1>
    static bool accepts_message(etl::message_id_t id2)
    {
      return Id1 == id2;
    }

    //********************************************
#include "private/diagnostic_uninitialized_push.h"
    void delete_current_message()
    {
      if (valid)
      {
        etl::imessage* pmsg = static_cast<etl::imessage*>(data);

        pmsg->~imessage();
      }
    }
#include "private/diagnostic_pop.h"

    //********************************************
    void add_new_message(const etl::imessage& msg)
    {
      (add_new_message_type<TMessageTypes>(msg) || ...);
    }

    //********************************************
    void add_new_message(etl::imessage&& msg)
    {
      (add_new_message_type<TMessageTypes>(etl::move(msg)) || ...);
    }

#include "private/diagnostic_uninitialized_push.h"
    //********************************************
    /// Only enabled for types that are in the typelist.
    //********************************************
    template <typename TMessage>
    etl::enable_if_t<etl::is_one_of_v<etl::remove_const_t<etl::remove_reference_t<TMessage>>, TMessageTypes...>, void>
      add_new_message_type(TMessage&& msg)
    {
      void* p = data;
      new (p) etl::remove_reference_t<TMessage>((etl::forward<TMessage>(msg)));
    }
#include "private/diagnostic_pop.h"

#include "private/diagnostic_uninitialized_push.h"
    //********************************************
    template <typename TType>
    bool add_new_message_type(const etl::imessage& msg)
    {
      if (TType::ID == msg.get_message_id())
      {
        void* p = data;
        new (p) TType(static_cast<const TType&>(msg));
        return true;
      }
      else
      {
        return false;
      }
    }
#include "private/diagnostic_pop.h"

    //********************************************
    template <typename TType>
    bool add_new_message_type(etl::imessage&& msg)
    {
      if (TType::ID == msg.get_message_id())
      {
        void* p = data;
        new (p) TType(static_cast<TType&&>(msg));
        return true;
      }
      else
      {
        return false;
      }
    }

    typename etl::aligned_storage<SIZE, ALIGNMENT>::type data;
    bool valid;
  };

#else

  /*[[[cog
    import cog

    ################################################
    def generate_accepts_return(n):
        cog.out("    return")
        for i in range(1, n + 1):
            cog.out(" T%d::ID == id" % i)
            if i < n:
                cog.out(" ||")
                if i % 4 == 0:
                    cog.outl("")
                    cog.out("          ")
        cog.outl(";")

    ################################################
    def generate_accepts_return_compile_time(n):
        cog.out("    return")
        for i in range(1, n + 1):
            cog.out(" T%d::ID == Id" % i)
            if i < n:
                cog.out(" ||")
                if i % 4 == 0:
                    cog.outl("")
                    cog.out("          ")
        cog.outl(";")

    ################################################
    def generate_accepts_return_compile_time_TMessage(n):
        cog.out("    return")
        for i in range(1, n + 1):
            cog.out(" T%d::ID == TMessage::ID" % i)
            if i < n:
                cog.out(" ||")
                if i % 4 == 0:
                    cog.outl("")
                    cog.out("          ")
        cog.outl(";")

    ################################################
    def generate_static_assert_cpp03(n):
        cog.outl("    // Not etl::message_packet, not etl::imessage and in typelist.")
        cog.out("    static const bool Enabled = (!etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::message_packet<")
        for i in range(1, n):
            cog.out("T%d, " % i)
        cog.outl("T%s> >::value &&" % n)
        cog.outl("                                 !etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::imessage>::value &&")
        cog.out("                                 etl::is_one_of<typename etl::remove_cvref<TMessage>::type,")
        for i in range(1, n):
            cog.out("T%d, " % i)
        cog.outl("T%s>::value);" % n)
        cog.outl("")
        cog.outl("    ETL_STATIC_ASSERT(Enabled, \"Message not in packet type list\");")

    ################################################
    def generate_static_assert_cpp11(n):
        cog.outl("    // Not etl::message_packet, not etl::imessage and in typelist.")
        cog.out("    static constexpr bool Enabled = (!etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::message_packet<")
        for i in range(1, n):
            cog.out("T%d, " % i)
        cog.outl("T%s> >::value &&" % n)
        cog.outl("                                     !etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::imessage>::value &&")
        cog.out("                                     etl::is_one_of<typename etl::remove_cvref<TMessage>::type,")
        for i in range(1, n):
            cog.out("T%d, " % i)
        cog.outl("T%s>::value);" % n)
        cog.outl("")
        cog.outl("    ETL_STATIC_ASSERT(Enabled, \"Message not in packet type list\");")

    ################################################
    # The first definition for all of the messages.
    ################################################
    cog.outl("//***************************************************************************")
    cog.outl("// The definition for all %s message types." % Handlers)
    cog.outl("//***************************************************************************")
    cog.out("template <")
    cog.out("typename T1, ")
    for n in range(2, int(Handlers)):
        cog.out("typename T%s = void, " % n)
        if n % 4 == 0:
            cog.outl("")
            cog.out("          ")
    cog.outl("typename T%s = void>" % int(Handlers))
    cog.outl("class message_packet")
    cog.outl("{")
    cog.outl("public:")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  message_packet()")
    cog.outl("    : valid(false)")
    cog.outl("  {")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  explicit message_packet(const etl::imessage& msg)")
    cog.outl("  {")
    cog.outl("    if (accepts(msg))")
    cog.outl("    {")
    cog.outl("      add_new_message(msg);")
    cog.outl("      valid = true;")
    cog.outl("    }")
    cog.outl("    else")
    cog.outl("    {")
    cog.outl("      valid = false;")
    cog.outl("    }")
    cog.outl("")
    cog.outl("    ETL_ASSERT(valid, ETL_ERROR(unhandled_message_exception));")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("")
    cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
    cog.outl("  //********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  explicit message_packet(etl::imessage&& msg)")
    cog.outl("  {")
    cog.outl("    if (accepts(msg))")
    cog.outl("    {")
    cog.outl("      add_new_message(etl::move(msg));")
    cog.outl("      valid = true;")
    cog.outl("    }")
    cog.outl("    else")
    cog.outl("    {")
    cog.outl("      valid = false;")
    cog.outl("    }")
    cog.outl("")
    cog.outl("    ETL_ASSERT(valid, ETL_ERROR(unhandled_message_exception));")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("#endif")
    cog.outl("")
    cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION) && !defined(ETL_COMPILER_GREEN_HILLS)")
    cog.outl("  //********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.out("  template <typename TMessage, typename = typename etl::enable_if<!etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::message_packet<")
    for n in range(1, int(Handlers)):
        cog.out("T%s, " % n)
    cog.outl("T%s> >::value &&" % int(Handlers))
    cog.outl("                                                                  !etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::imessage>::value &&")
    cog.out("                                                                  !etl::is_one_of<typename etl::remove_cvref<TMessage>::type, ")
    for n in range(1, int(Handlers)):
        cog.out("T%s, " % n)
    cog.outl("T%s>::value, int>::type>" % int(Handlers))
    cog.outl("  explicit message_packet(TMessage&& /*msg*/)")
    cog.outl("    : valid(true)")
    cog.outl("  {")
    generate_static_assert_cpp11(int(Handlers))
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("#else")
    cog.outl("  //********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  template <typename TMessage>")
    cog.out("  explicit message_packet(const TMessage& /*msg*/, typename etl::enable_if<!etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::message_packet<")
    for n in range(1, int(Handlers)):
        cog.out("T%s, " % n)
    cog.outl("T%s> >::value &&" % int(Handlers))
    cog.outl("                                                                       !etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::imessage>::value &&")
    cog.out("                                                                       !etl::is_one_of<typename etl::remove_cvref<TMessage>::type, ")
    for n in range(1, int(Handlers)):
        cog.out("T%s, " % n)
    cog.outl("T%s>::value, int>::type = 0)" % int(Handlers))
    cog.outl("    : valid(true)")
    cog.outl("  {")
    generate_static_assert_cpp03(int(Handlers))
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("#endif")
    cog.outl("")
    cog.outl("  //**********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  message_packet(const message_packet& other)")
    cog.outl("    : valid(other.is_valid())")
    cog.outl("  {")
    cog.outl("    if (valid)")
    cog.outl("    {")
    cog.outl("      add_new_message(other.get());")
    cog.outl("    }")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("")
    cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
    cog.outl("  //**********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  message_packet(message_packet&& other)")
    cog.outl("    : valid(other.is_valid())")
    cog.outl("  {")
    cog.outl("    if (valid)")
    cog.outl("    {")
    cog.outl("      add_new_message(etl::move(other.get()));")
    cog.outl("    }")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("#endif")
    cog.outl("")
    cog.outl("  //**********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  message_packet& operator =(const message_packet& rhs)")
    cog.outl("  {")
    cog.outl("    delete_current_message();")
    cog.outl("    valid = rhs.is_valid();")
    cog.outl("    if (valid)")
    cog.outl("    {")
    cog.outl("      add_new_message(rhs.get());")
    cog.outl("    }")
    cog.outl("")
    cog.outl("    return *this;")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("")
    cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
    cog.outl("  //**********************************************")
    cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  message_packet& operator =(message_packet&& rhs)")
    cog.outl("  {")
    cog.outl("    delete_current_message();")
    cog.outl("    valid = rhs.is_valid();")
    cog.outl("    if (valid)")
    cog.outl("    {")
    cog.outl("      add_new_message(etl::move(rhs.get()));")
    cog.outl("    }")
    cog.outl("")
    cog.outl("    return *this;")
    cog.outl("  }")
    cog.outl("#include \"private/diagnostic_pop.h\"")
    cog.outl("#endif")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("  ~message_packet()")
    cog.outl("  {")
    cog.outl("    delete_current_message();")
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("  etl::imessage& get() ETL_NOEXCEPT")
    cog.outl("  {")
    cog.outl("    return *static_cast<etl::imessage*>(data);")
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("  const etl::imessage& get() const ETL_NOEXCEPT")
    cog.outl("  {")
    cog.outl("    return *static_cast<const etl::imessage*>(data);")
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("  bool is_valid() const")
    cog.outl("  {")
    cog.outl("    return valid;")
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //**********************************************")
    cog.outl("  static ETL_CONSTEXPR bool accepts(etl::message_id_t id)")
    cog.outl("  {")
    generate_accepts_return(int(Handlers))
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //**********************************************")
    cog.outl("  static ETL_CONSTEXPR bool accepts(const etl::imessage& msg)")
    cog.outl("  {")
    cog.outl("    return accepts(msg.get_message_id());")
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //**********************************************")
    cog.outl("  template <etl::message_id_t Id>")
    cog.outl("  static ETL_CONSTEXPR bool accepts()")
    cog.outl("  {")
    generate_accepts_return_compile_time(int(Handlers))
    cog.outl("  }")
    cog.outl("")
    cog.outl("  //**********************************************")
    cog.outl("  template <typename TMessage>")
    cog.outl("  static ETL_CONSTEXPR")
    cog.outl("  typename etl::enable_if<etl::is_base_of<etl::imessage, TMessage>::value, bool>::type")
    cog.outl("    accepts()")
    cog.outl("  {")
    generate_accepts_return_compile_time_TMessage(int(Handlers))
    cog.outl("  }")
    cog.outl("")
    cog.outl("  enum")
    cog.outl("  {")
    cog.out("    SIZE      = etl::largest<")
    for n in range(1, int(Handlers)):
        cog.out("T%d, " % n)
    cog.outl("T%s>::size," % int(Handlers))
    cog.out("    ALIGNMENT = etl::largest<")
    for n in range(1, int(Handlers)):
        cog.out("T%d, " % n)
    cog.outl("T%s>::alignment" % int(Handlers))
    cog.outl("  };")
    cog.outl("")
    cog.outl("private:")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("  #include \"private/diagnostic_uninitialized_push.h\"")
    cog.outl("  void delete_current_message()")
    cog.outl("  {")
    cog.outl("    if (valid)")
    cog.outl("    {")
    cog.outl("      etl::imessage* pmsg = static_cast<etl::imessage*>(data);")
    cog.outl("")
    cog.outl("      pmsg->~imessage();")
    cog.outl("    }")
    cog.outl("  }")
    cog.outl("  #include \"private/diagnostic_pop.h\"")
    cog.outl("")
    cog.outl("  //********************************************")
    cog.outl("  void add_new_message(const etl::imessage& msg)")
    cog.outl("  {")
    cog.outl("    const size_t id = msg.get_message_id();")
    cog.outl("    void* p = data;")
    cog.outl("")
    cog.outl("    switch (id)")
    cog.outl("    {")
    for n in range(1, int(Handlers) + 1):
        cog.outl("      case T%d::ID: ::new (p) T%d(static_cast<const T%d&>(msg)); break;" %(n, n, n))
    cog.outl("      default: ETL_ASSERT(false, ETL_ERROR(unhandled_message_exception)); break;")
    cog.outl("    }")
    cog.outl("  }")
    cog.outl("")
    cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
    cog.outl("  //********************************************")
    cog.outl("  void add_new_message(etl::imessage&& msg)")
    cog.outl("  {")
    cog.outl("    const size_t id = msg.get_message_id();")
    cog.outl("    void* p = data;")
    cog.outl("")
    cog.outl("    switch (id)")
    cog.outl("    {")
    for n in range(1, int(Handlers) + 1):
        cog.outl("      case T%d::ID: ::new (p) T%d(static_cast<T%d&&>(msg)); break;" %(n, n, n))
    cog.outl("      default: ETL_ASSERT(false, ETL_ERROR(unhandled_message_exception)); break;")
    cog.outl("    }")
    cog.outl("  }")
    cog.outl("#endif")
    cog.outl("")
    cog.outl("  typename etl::aligned_storage<SIZE, ALIGNMENT>::type data;")
    cog.outl("  bool valid;")
    cog.outl("};")

    ####################################
    # All of the other specialisations.
    ####################################
    for n in range(int(Handlers) - 1, 0, -1):
        cog.outl("")
        cog.outl("//***************************************************************************")
        if n == 1:
            cog.outl("// Specialisation for %d message type." % n)
        else:
            cog.outl("// Specialisation for %d message types." % n)
        cog.outl("//***************************************************************************")
        cog.out("template <")
        for t in range(1, n):
            cog.out("typename T%s, " % t)
            if t % 4 == 0:
                cog.outl("")
                cog.out("          ")
        cog.outl("typename T%s>" % n)
        cog.out("class message_packet<")
        for t in range(1, n + 1):
            cog.out("T%d, " % t)
            if t % 16 == 0:
                cog.outl("")
                cog.out("               ")
        for t in range(n + 1, int(Handlers)):
            cog.out("void, ")
            if t % 16 == 0:
                cog.outl("")
                cog.out("               ")
        cog.outl("void>")
        cog.outl("{")
        cog.outl("public:")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  message_packet()")
        cog.outl("    : valid(false)")
        cog.outl("  {")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  explicit message_packet(const etl::imessage& msg)")
        cog.outl("  {")
        cog.outl("    if (accepts(msg))")
        cog.outl("    {")
        cog.outl("      add_new_message(msg);")
        cog.outl("      valid = true;")
        cog.outl("    }")
        cog.outl("    else")
        cog.outl("    {")
        cog.outl("      valid = false;")
        cog.outl("    }")
        cog.outl("")
        cog.outl("    ETL_ASSERT(valid, ETL_ERROR(unhandled_message_exception));")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("")
        cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
        cog.outl("  //********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  explicit message_packet(etl::imessage&& msg)")
        cog.outl("  {")
        cog.outl("    if (accepts(msg))")
        cog.outl("    {")
        cog.outl("      add_new_message(etl::move(msg));")
        cog.outl("      valid = true;")
        cog.outl("    }")
        cog.outl("    else")
        cog.outl("    {")
        cog.outl("      valid = false;")
        cog.outl("    }")
        cog.outl("")
        cog.outl("    ETL_ASSERT(valid, ETL_ERROR(unhandled_message_exception));")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("#endif")
        cog.outl("")
        cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION) && !defined(ETL_COMPILER_GREEN_HILLS)")
        cog.outl("  //********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.out("  template <typename TMessage, typename = typename etl::enable_if<!etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::message_packet<")
        for t in range(1, n):
            cog.out("T%s, " % t)
        cog.outl("T%s> >::value &&" % n)
        cog.outl("                                                                  !etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::imessage>::value &&")
        cog.out("                                                                  !etl::is_one_of<typename etl::remove_cvref<TMessage>::type, ")
        for t in range(1, n):
            cog.out("T%s, " % t)
        cog.outl("T%s>::value, int>::type>" % n)
        cog.outl("  explicit message_packet(TMessage&& /*msg*/)")
        cog.outl("    : valid(true)")
        cog.outl("  {")
        generate_static_assert_cpp11(n)
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("#else")
        cog.outl("  //********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  template <typename TMessage>")
        cog.out("  explicit message_packet(const TMessage& /*msg*/, typename etl::enable_if<!etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::message_packet<")
        for t in range(1, n):
            cog.out("T%s, " % t)
        cog.outl("T%s> >::value &&" % n)
        cog.outl("                                                                       !etl::is_same<typename etl::remove_cvref<TMessage>::type, etl::imessage>::value &&")
        cog.out("                                                                       !etl::is_one_of<typename etl::remove_cvref<TMessage>::type, ")
        for t in range(1, n):
            cog.out("T%s, " % t)
        cog.outl("T%s>::value, int>::type = 0)" % n)
        cog.outl("    : valid(true)")
        cog.outl("  {")
        generate_static_assert_cpp03(n)
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("#endif")
        cog.outl("")
        cog.outl("  //**********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  message_packet(const message_packet& other)")
        cog.outl("    : valid(other.is_valid())")
        cog.outl("  {")
        cog.outl("    if (valid)")
        cog.outl("    {")
        cog.outl("      add_new_message(other.get());")
        cog.outl("    }")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("")
        cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
        cog.outl("  //**********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  message_packet(message_packet&& other)")
        cog.outl("    : valid(other.is_valid())")
        cog.outl("  {")
        cog.outl("    if (valid)")
        cog.outl("    {")
        cog.outl("      add_new_message(etl::move(other.get()));")
        cog.outl("    }")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("#endif")
        cog.outl("")
        cog.outl("  //**********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  message_packet& operator =(const message_packet& rhs)")
        cog.outl("  {")
        cog.outl("    delete_current_message();")
        cog.outl("    valid = rhs.is_valid();")
        cog.outl("    if (valid)")
        cog.outl("    {")
        cog.outl("      add_new_message(rhs.get());")
        cog.outl("    }")
        cog.outl("")
        cog.outl("    return *this;")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("")
        cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
        cog.outl("  //**********************************************")
        cog.outl("#include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  message_packet& operator =(message_packet&& rhs)")
        cog.outl("  {")
        cog.outl("    delete_current_message();")
        cog.outl("    valid = rhs.is_valid();")
        cog.outl("    if (valid)")
        cog.outl("    {")
        cog.outl("      add_new_message(etl::move(rhs.get()));")
        cog.outl("    }")
        cog.outl("")
        cog.outl("    return *this;")
        cog.outl("  }")
        cog.outl("#include \"private/diagnostic_pop.h\"")
        cog.outl("#endif")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("  ~message_packet()")
        cog.outl("  {")
        cog.outl("    delete_current_message();")
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("  etl::imessage& get() ETL_NOEXCEPT")
        cog.outl("  {")
        cog.outl("    return *static_cast<etl::imessage*>(data);")
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("  const etl::imessage& get() const ETL_NOEXCEPT")
        cog.outl("  {")
        cog.outl("    return *static_cast<const etl::imessage*>(data);")
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("  bool is_valid() const")
        cog.outl("  {")
        cog.outl("    return valid;")
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //**********************************************")
        cog.outl("  static ETL_CONSTEXPR bool accepts(etl::message_id_t id)")
        cog.outl("  {")
        generate_accepts_return(n)
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //**********************************************")
        cog.outl("  static ETL_CONSTEXPR bool accepts(const etl::imessage& msg)")
        cog.outl("  {")
        cog.outl("    return accepts(msg.get_message_id());")
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //**********************************************")
        cog.outl("  template <etl::message_id_t Id>")
        cog.outl("  static ETL_CONSTEXPR bool accepts()")
        cog.outl("  {")
        generate_accepts_return_compile_time(n)
        cog.outl("  }")
        cog.outl("")
        cog.outl("  //**********************************************")
        cog.outl("  template <typename TMessage>")
        cog.outl("  static ETL_CONSTEXPR")
        cog.outl("  typename etl::enable_if<etl::is_base_of<etl::imessage, TMessage>::value, bool>::type")
        cog.outl("    accepts()")
        cog.outl("  {")
        generate_accepts_return_compile_time_TMessage(n)
        cog.outl("  }")
        cog.outl("")
        cog.outl("  enum")
        cog.outl("  {")
        cog.out("    SIZE      = etl::largest<")
        for t in range(1, n):
            cog.out("T%d, " % t)
        cog.outl("T%s>::size," % n)
        cog.out("    ALIGNMENT = etl::largest<")
        for t in range(1, n):
            cog.out("T%d, " % t)
        cog.outl("T%s>::alignment" % n)
        cog.outl("  };")
        cog.outl("")
        cog.outl("private:")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("  #include \"private/diagnostic_uninitialized_push.h\"")
        cog.outl("  void delete_current_message()")
        cog.outl("  {")
        cog.outl("    if (valid)")
        cog.outl("    {")
        cog.outl("      etl::imessage* pmsg = static_cast<etl::imessage*>(data);")
        cog.outl("")
        cog.outl("      pmsg->~imessage();")
        cog.outl("    }")
        cog.outl("  }")
        cog.outl("  #include \"private/diagnostic_pop.h\"")
        cog.outl("")
        cog.outl("  //********************************************")
        cog.outl("  void add_new_message(const etl::imessage& msg)")
        cog.outl("  {")
        cog.outl("    const size_t id = msg.get_message_id();")
        cog.outl("    void* p = data;")
        cog.outl("")
        cog.outl("    switch (id)")
        cog.outl("    {")
        for t in range(1, n + 1):
            cog.outl("      case T%d::ID: ::new (p) T%d(static_cast<const T%d&>(msg)); break;" %(t, t, t))
        cog.outl("      default: break;")
        cog.outl("    }")
        cog.outl("  }")
        cog.outl("")
        cog.outl("#if ETL_USING_CPP11 && !defined(ETL_MESSAGE_PACKET_FORCE_CPP03_IMPLEMENTATION)")
        cog.outl("  //********************************************")
        cog.outl("  void add_new_message(etl::imessage&& msg)")
        cog.outl("  {")
        cog.outl("    const size_t id = msg.get_message_id();")
        cog.outl("    void* p = data;")
        cog.outl("")
        cog.outl("    switch (id)")
        cog.outl("    {")
        for t in range(1, n + 1):
            cog.outl("      case T%d::ID: ::new (p) T%d(static_cast<T%d&&>(msg)); break;" %(t, t, t))
        cog.outl("      default: break;")
        cog.outl("    }")
        cog.outl("  }")
        cog.outl("#endif")
        cog.outl("")
        cog.outl("  typename etl::aligned_storage<SIZE, ALIGNMENT>::type data;")
        cog.outl("  bool valid;")
        cog.outl("};")
  ]]]*/
  /*[[[end]]]*/
#endif
}
#else
  #error "etl::message_packet is not compatible with non-virtual etl::imessage"
#endif

#endif
