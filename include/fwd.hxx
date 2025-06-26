#pragma once

/**
 * \file
 *
 * This header provides format declarations for most of libcosmos's types.
 **/

namespace cosmos {

template <typename ENUM>
class BitMask;

class GroupInfo;
class PasswdInfo;
template <typename DB_STRUCT>
class InfoBase;

struct SysString;

struct Init;

class ApiError;
class CosmosError;
class FileError;
class InternalError;
class RangeError;
class ResolveError;
class RuntimeError;
class UsageError;
class WouldBlock;

class DirEntry;
class DirFD;
class DirIterator;
class DirStream;
class Directory;
class FDFile;
class File;
class FileBase;
class FileDescriptor;
class FileLock;
class FileStatus;
class TempDir;
class TempFile;
class FileType;
class FileMode;

class EventFile;
class ILogger;
class MemFile;
class Pipe;
class Poller;
class SecretFile;
class StdLogger;
template <typename STREAM_TYPE>
class StreamAdaptor;
class InputStreamAdaptor;
class OutputStreamAdaptor;
class InputOutputStreamAdaptor;
class StreamIO;
class Terminal;
struct InputMemoryRegion;
struct OutputMemoryRegion;
template <typename MEMORY_REGION>
class IOVector;

class AddressHints;
class AddressInfo;
struct AddressInfoIterator;
class AddressInfoList;
class IPAddressBase;
class IP4Address;
class IP6Address;
class IP4Options;
class IP6Options;
class InterfaceAddress;
class InterfaceAddressIterator;
class InterfaceAddressList;
class InterfaceEnumerator;
struct InterfaceInfo;
class InterfaceIterator;
class LinkLayerAddress;
class ListenSocket;
class Socket;
class SocketAddress;
class SocketOptions;
class TCPOptions;
class UDPOptions;
class UnixAddress;
class UnixClientSocket;
class UnixConnection;
class UnixDatagramSocket;
class UnixListenSocket;
class UnixOptions;
class SendMessageHeader;
class ReceiveMessageHeader;
struct TCPInfo;
struct UnixCredentials;
class UnixRightsMessage;
class UnixCredentialsMessage;

class ChildCloner;
class Mapping;
class PidFD;
class ProcessFile;
class SchedulerSettings;
class OtherSchedulerSettings;
class RealTimeSchedulerSettings;
class FifoSchedulerSettings;
class RoundRobinSchedulerSettings;
class SigAction;
class SigInfo;
class SigSet;
class SignalFD;
class SubProc;
class Tracee;
class WaitStatus;
class Signal;

class Condition;
class ConditionMutex;
class Mutex;
class PosixThread;
class RWLock;

} // end ns
