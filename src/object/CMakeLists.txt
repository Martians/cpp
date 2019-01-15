# cmake_minimum_required(VERSION <specify CMake version here>)
project(object)

set(CMAKE_CXX_STANDARD 14)
cmake_minimum_required(VERSION 3.5)

include_directories(src)
include_directories(../../include)

include_directories(src/Advance)
include_directories(src/Advance/Buffer)
include_directories(src/Advance/Temporary)
include_directories(src/Applet/Client)
include_directories(src/CodeHelper)
include_directories(src/Common)
include_directories(src/Perform)
include_directories(src/Sample/thrift/gen-cpp)
include_directories(src/Service/ObjectService)
include_directories(src/Service/ObjectService/ObjectClient)
include_directories(src/Service/ObjectService/UnitTest)
#include_directories(src/thrift)
#include_directories(src/thrift/async)
#include_directories(src/thrift/concurrency)
#include_directories(src/thrift/processor)
#include_directories(src/thrift/protocol)
#include_directories(src/thrift/qt)
#include_directories(src/thrift/server)
#include_directories(src/thrift/transport)
#include_directories(src/thrift/tutorial/gen-cpp)
#include_directories(src/thrift/windows)

add_executable(object
        src/Advance/Buffer/ByteBuffer.cpp
        src/Advance/Buffer/ByteBuffer.hpp
        src/Advance/Buffer/CycleBuffer.cpp
        src/Advance/Buffer/CycleBuffer.hpp
        src/Advance/Temporary/Array.hpp
        src/Advance/AppHelper.cpp
        src/Advance/AppHelper.hpp
        src/Advance/Barrier.hpp
        src/Advance/BaseAlloter.cpp
        src/Advance/BaseAlloter.hpp
        src/Advance/BaseBuffer.cpp
        src/Advance/BaseBuffer.hpp
        src/Advance/BufferStream.hpp
        src/Advance/ByteOrder.hpp
        src/Advance/Container.hpp
        src/Advance/Coordinator.hpp
        src/Advance/Define.hpp
        src/Advance/DynBuffer.cpp
        src/Advance/DynBuffer.hpp
        src/Advance/DynChunk.hpp
        src/Advance/FastHash.cpp
        src/Advance/FastHash.hpp
        src/Advance/Functional.hpp
        src/Advance/List.cpp
        src/Advance/List.hpp
        src/Advance/MemPool.cpp
        src/Advance/MemPool.hpp
        src/Advance/Pointer.hpp
        src/Advance/Simple.cpp
        src/Advance/SingleList.hpp
        src/Advance/Singleton.hpp
        src/Advance/TypeAlloter.hpp
        src/Advance/Util.hpp
        src/Advance/WrapAlloter.hpp
        src/Applet/Client/Sample/TestClient.cpp
        src/Applet/Client/Sample/TestCommand.cpp
        src/Applet/Client/ClientBase.cpp
        src/Applet/Client/ClientBase.hpp
        src/Applet/Client/ClientManage.cpp
        src/Applet/Client/ClientManage.hpp
        src/Applet/Client/Command.cpp
        src/Applet/Client/Command.hpp
        src/Applet/Client/ConfigBase.hpp
        src/Applet/Client/Interface.hpp
        src/CodeHelper/Bitset.hpp
        src/CodeHelper/Refer.hpp
        src/CodeHelper/RunStat.hpp
        src/CodeHelper/Trigger.hpp
        src/CodeHelper/Validate.hpp
        src/Common/Atomic.hpp
        src/Common/CodeHelper.hpp
        src/Common/Common.hpp
        src/Common/Cond.hpp
        src/Common/Const.hpp
        src/Common/Container.hpp
        src/Common/DataType.hpp
        src/Common/Define.hpp
        src/Common/Display.cpp
        src/Common/Display.hpp
        src/Common/File.cpp
        src/Common/File.hpp
        src/Common/Global.hpp
        src/Common/Logger.cpp
        src/Common/Logger.hpp
        src/Common/LogHelper.hpp
        src/Common/Main.cxx
        src/Common/Mutex.hpp
        src/Common/Reference.cpp
        src/Common/Reference.hpp
        src/Common/Simple.cpp
        src/Common/String.cpp
        src/Common/String.hpp
        src/Common/Thread.hpp
        src/Common/ThreadBase.hpp
        src/Common/ThreadInfo.cpp
        src/Common/ThreadInfo.hpp
        src/Common/ThreadPool.cpp
        src/Common/ThreadPool.hpp
        src/Common/Time.hpp
        src/Common/Type.hpp
        src/Common/TypeQueue.hpp
        src/Common/Util.hpp
        src/Perform/Debug.hpp
        src/Perform/LogHelper.hpp
        src/Perform/Mock.hpp
        src/Perform/MockBase.cpp
        src/Perform/MockBase.hpp
        src/Perform/Random.cpp
        src/Perform/Random.hpp
        src/Perform/Source.cpp
        src/Perform/Source.hpp
        src/Perform/Statistic.hpp
        src/Perform/StatThread.cpp
        src/Perform/StatThread.hpp
        src/Perform/TestUtil.cpp
        src/Perform/TestUtil.hpp
        src/Perform/Timer.cpp
        src/Perform/Timer.hpp
#        src/Sample/thrift/gen-cpp/test_constants.cpp
#        src/Sample/thrift/gen-cpp/test_constants.h
#        src/Sample/thrift/gen-cpp/test_types.cpp
#        src/Sample/thrift/gen-cpp/test_types.h
#        src/Sample/thrift/gen-cpp/WorkService.cpp
#        src/Sample/thrift/gen-cpp/WorkService.h
#        src/Sample/thrift/gen-cpp/WorkService_async_server.skeleton.cpp
#        src/Sample/thrift/WorkClient.cpp
#        src/Sample/thrift/WorkServer.cpp
#        src/Service/ObjectService/ObjectClient/ObjectClient.cpp
#        src/Service/ObjectService/ObjectClient/ObjectClient.hpp
#        src/Service/ObjectService/ObjectClient/ObjectTest.cpp
#        src/Service/ObjectService/ObjectClient/ObjectTest.hpp
#        src/Service/ObjectService/UnitTest/Mock.hpp
#        src/Service/ObjectService/UnitTest/ObjectUnitTest.hpp
#        src/Service/ObjectService/UnitTest/ReaderUnitTest.cpp
#        src/Service/ObjectService/UnitTest/ReaderUnitTest.hpp
#        src/Service/ObjectService/UnitTest/WriterUnitTest.cpp
#        src/Service/ObjectService/UnitTest/WriterUnitTest.hpp
#        src/Service/ObjectService/Config.hpp
#        src/Service/ObjectService/Control.cpp
#        src/Service/ObjectService/Control.hpp
#        src/Service/ObjectService/Logger.hpp
#        src/Service/ObjectService/LogReader.hpp
#        src/Service/ObjectService/LogWriter.hpp
#        src/Service/ObjectService/Main.cpp
#        src/Service/ObjectService/Object.cpp
#        src/Service/ObjectService/Object.hpp
#        src/Service/ObjectService/ObjectUnit.cpp
#        src/Service/ObjectService/ObjectUnit.hpp
#        src/Service/ObjectService/PostgreSQL.cpp
#        src/Service/ObjectService/PostgreSQL.hpp
#        src/Service/ObjectService/Reader.cpp
#        src/Service/ObjectService/Reader.hpp
#        src/Service/ObjectService/Statistic.hpp
#        src/Service/ObjectService/Writer.cpp
#        src/Service/ObjectService/Writer.hpp
#        src/thrift/async/TAsyncBufferProcessor.h
#        src/thrift/async/TAsyncChannel.cpp
#        src/thrift/async/TAsyncChannel.h
#        src/thrift/async/TAsyncDispatchProcessor.h
#        src/thrift/async/TAsyncProcessor.h
#        src/thrift/async/TAsyncProtocolProcessor.cpp
#        src/thrift/async/TAsyncProtocolProcessor.h
#        src/thrift/async/TConcurrentClientSyncInfo.cpp
#        src/thrift/async/TConcurrentClientSyncInfo.h
#        src/thrift/async/TEvhttpClientChannel.cpp
#        src/thrift/async/TEvhttpClientChannel.h
#        src/thrift/async/TEvhttpServer.cpp
#        src/thrift/async/TEvhttpServer.h
#        src/thrift/concurrency/BoostMonitor.cpp
#        src/thrift/concurrency/BoostMutex.cpp
#        src/thrift/concurrency/BoostThreadFactory.cpp
#        src/thrift/concurrency/BoostThreadFactory.h
#        src/thrift/concurrency/Exception.h
#        src/thrift/concurrency/FunctionRunner.h
#        src/thrift/concurrency/Monitor.cpp
#        src/thrift/concurrency/Monitor.h
#        src/thrift/concurrency/Mutex.cpp
#        src/thrift/concurrency/Mutex.h
#        src/thrift/concurrency/PlatformThreadFactory.h
#        src/thrift/concurrency/PosixThreadFactory.cpp
#        src/thrift/concurrency/PosixThreadFactory.h
#        src/thrift/concurrency/StdMonitor.cpp
#        src/thrift/concurrency/StdMutex.cpp
#        src/thrift/concurrency/StdThreadFactory.cpp
#        src/thrift/concurrency/StdThreadFactory.h
#        src/thrift/concurrency/Thread.h
#        src/thrift/concurrency/ThreadManager.cpp
#        src/thrift/concurrency/ThreadManager.h
#        src/thrift/concurrency/TimerManager.cpp
#        src/thrift/concurrency/TimerManager.h
#        src/thrift/concurrency/Util.cpp
#        src/thrift/concurrency/Util.h
#        src/thrift/processor/PeekProcessor.cpp
#        src/thrift/processor/PeekProcessor.h
#        src/thrift/processor/StatsProcessor.h
#        src/thrift/processor/TMultiplexedProcessor.h
#        src/thrift/protocol/TBase64Utils.cpp
#        src/thrift/protocol/TBase64Utils.h
#        src/thrift/protocol/TBinaryProtocol.h
#        src/thrift/protocol/TBinaryProtocol.tcc
#        src/thrift/protocol/TCompactProtocol.h
#        src/thrift/protocol/TCompactProtocol.tcc
#        src/thrift/protocol/TDebugProtocol.cpp
#        src/thrift/protocol/TDebugProtocol.h
#        src/thrift/protocol/THeaderProtocol.h
#        src/thrift/protocol/TJSONProtocol.cpp
#        src/thrift/protocol/TJSONProtocol.h
#        src/thrift/protocol/TMultiplexedProtocol.cpp
#        src/thrift/protocol/TMultiplexedProtocol.h
#        src/thrift/protocol/TProtocol.cpp
#        src/thrift/protocol/TProtocol.h
#        src/thrift/protocol/TProtocolDecorator.h
#        src/thrift/protocol/TProtocolException.h
#        src/thrift/protocol/TProtocolTap.h
#        src/thrift/protocol/TProtocolTypes.h
#        src/thrift/protocol/TVirtualProtocol.h
#        src/thrift/qt/TQIODeviceTransport.cpp
#        src/thrift/qt/TQIODeviceTransport.h
#        src/thrift/qt/TQTcpServer.cpp
#        src/thrift/qt/TQTcpServer.h
#        src/thrift/server/TConnectedClient.cpp
#        src/thrift/server/TConnectedClient.h
#        src/thrift/server/TNonblockingServer.cpp
#        src/thrift/server/TNonblockingServer.h
#        src/thrift/server/TServer.cpp
#        src/thrift/server/TServer.h
#        src/thrift/server/TServerFramework.cpp
#        src/thrift/server/TServerFramework.h
#        src/thrift/server/TSimpleServer.cpp
#        src/thrift/server/TSimpleServer.h
#        src/thrift/server/TThreadedServer.cpp
#        src/thrift/server/TThreadedServer.h
#        src/thrift/server/TThreadPoolServer.cpp
#        src/thrift/server/TThreadPoolServer.h
#        src/thrift/transport/PlatformSocket.h
#        src/thrift/transport/TBufferTransports.cpp
#        src/thrift/transport/TBufferTransports.h
#        src/thrift/transport/TFDTransport.cpp
#        src/thrift/transport/TFDTransport.h
#        src/thrift/transport/TFileTransport.cpp
#        src/thrift/transport/TFileTransport.h
#        src/thrift/transport/THeaderTransport.h
#        src/thrift/transport/THttpClient.cpp
#        src/thrift/transport/THttpClient.h
#        src/thrift/transport/THttpServer.cpp
#        src/thrift/transport/THttpServer.h
#        src/thrift/transport/THttpTransport.cpp
#        src/thrift/transport/THttpTransport.h
#        src/thrift/transport/TPipe.cpp
#        src/thrift/transport/TPipe.h
#        src/thrift/transport/TPipeServer.cpp
#        src/thrift/transport/TPipeServer.h
#        src/thrift/transport/TServerSocket.cpp
#        src/thrift/transport/TServerSocket.h
#        src/thrift/transport/TServerTransport.h
#        src/thrift/transport/TShortReadTransport.h
#        src/thrift/transport/TSimpleFileTransport.cpp
#        src/thrift/transport/TSimpleFileTransport.h
#        src/thrift/transport/TSocket.cpp
#        src/thrift/transport/TSocket.h
#        src/thrift/transport/TSocketPool.cpp
#        src/thrift/transport/TSocketPool.h
#        src/thrift/transport/TSSLServerSocket.cpp
#        src/thrift/transport/TSSLServerSocket.h
#        src/thrift/transport/TSSLSocket.cpp
#        src/thrift/transport/TSSLSocket.h
#        src/thrift/transport/TTransport.h
#        src/thrift/transport/TTransportException.cpp
#        src/thrift/transport/TTransportException.h
#        src/thrift/transport/TTransportUtils.cpp
#        src/thrift/transport/TTransportUtils.h
#        src/thrift/transport/TVirtualTransport.h
#        src/thrift/transport/TZlibTransport.cpp
#        src/thrift/transport/TZlibTransport.h
#        src/thrift/tutorial/cpp/CppClient.cpp
#        src/thrift/tutorial/cpp/CppServer.cpp
#        src/thrift/tutorial/gen-cpp/Calculator.cpp
#        src/thrift/tutorial/gen-cpp/Calculator.h
#        src/thrift/tutorial/gen-cpp/Calculator_server.skeleton.cpp
#        src/thrift/tutorial/gen-cpp/shared_constants.cpp
#        src/thrift/tutorial/gen-cpp/shared_constants.h
#        src/thrift/tutorial/gen-cpp/shared_types.cpp
#        src/thrift/tutorial/gen-cpp/shared_types.h
#        src/thrift/tutorial/gen-cpp/SharedService.cpp
#        src/thrift/tutorial/gen-cpp/SharedService.h
#        src/thrift/tutorial/gen-cpp/SharedService_server.skeleton.cpp
#        src/thrift/tutorial/gen-cpp/tutorial_constants.cpp
#        src/thrift/tutorial/gen-cpp/tutorial_constants.h
#        src/thrift/tutorial/gen-cpp/tutorial_types.cpp
#        src/thrift/tutorial/gen-cpp/tutorial_types.h
#        src/thrift/windows/config.h
#        src/thrift/windows/GetTimeOfDay.cpp
#        src/thrift/windows/GetTimeOfDay.h
#        src/thrift/windows/Operators.h
#        src/thrift/windows/OverlappedSubmissionThread.cpp
#        src/thrift/windows/OverlappedSubmissionThread.h
#        src/thrift/windows/SocketPair.cpp
#        src/thrift/windows/SocketPair.h
#        src/thrift/windows/Sync.h
#        src/thrift/windows/TWinsockSingleton.cpp
#        src/thrift/windows/TWinsockSingleton.h
#        src/thrift/windows/WinFcntl.cpp
#        src/thrift/windows/WinFcntl.h
#        src/thrift/config.h
#        src/thrift/cxxfunctional.h
#        src/thrift/TApplicationException.cpp
#        src/thrift/TApplicationException.h
#        src/thrift/TBase.h
#        src/thrift/TDispatchProcessor.h
#        src/thrift/thrift-config.h
#        src/thrift/Thrift.h
#        src/thrift/TLogging.h
#        src/thrift/TOutput.cpp
#        src/thrift/TOutput.h
#        src/thrift/TProcessor.h
#        src/thrift/TToString.h
#        src/thrift/VirtualProfiling.cpp
        src/CMakeLists.txt)
