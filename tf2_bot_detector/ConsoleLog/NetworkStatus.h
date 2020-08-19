#pragma once

#include "ConsoleLog/IConsoleLine.h"

#include <string>
#include <string_view>

namespace tf2_bot_detector
{
	enum class SocketType : uint8_t
	{
		Client,
		Server,
		HLTV,
		Matchmaking,
		SystemLink,
		LAN,

		COUNT,
	};

	struct SplitPacket
	{
		SplitPacket() = default;
		SplitPacket(SocketType socketType, uint8_t index, uint8_t count, uint16_t sequence, uint16_t size,
			uint16_t mtu, std::string addr, uint16_t totalSize) :
			m_SocketType(socketType), m_Index(index), m_Count(count), m_Sequence(sequence), m_Size(size),
			m_MTU(mtu), m_Address(std::move(addr)), m_TotalSize(totalSize)
		{
		}

		SocketType m_SocketType{};
		uint8_t m_Index{};
		uint8_t m_Count{};
		uint16_t m_Sequence{};
		uint16_t m_Size{};
		uint16_t m_MTU{};
		uint16_t m_TotalSize{};
		std::string m_Address;
	};

	class SplitPacketLine final : public ConsoleLineBase<SplitPacketLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		SplitPacketLine(time_point_t timestamp, SplitPacket packet);

		static std::shared_ptr<IConsoleLine> TryParse(const std::string_view& text, time_point_t timestamp);

		const SplitPacket& GetSplitPacket() const { return m_Packet; }

		ConsoleLineType GetType() const override { return ConsoleLineType::SplitPacket; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

	private:
		SplitPacket m_Packet;
	};

	class NetStatusConfigLine final : public ConsoleLineBase<NetStatusConfigLine>
	{
		using BaseClass = ConsoleLineBase;

	public:
		enum class PlayerMode : uint8_t
		{
			Multiplayer,
			Singleplayer,
		};

		enum class ServerMode : uint8_t
		{
			Dedicated,
			Listen,
		};

		NetStatusConfigLine(time_point_t timestamp, PlayerMode playerMode, ServerMode serverMode, unsigned connectionCount);
		static std::shared_ptr<IConsoleLine> TryParse(const std::string_view& text, time_point_t timestamp);

		ConsoleLineType GetType() const override { return ConsoleLineType::NetStatusConfig; }
		bool ShouldPrint() const override { return false; }
		void Print(const PrintArgs& args) const override;

		unsigned GetConnectionCount() const { return m_ConnectionCount; }
		PlayerMode GetPlayerMode() const { return m_PlayerMode; }
		ServerMode GetServerMode() const { return m_ServerMode; }

	private:
		unsigned m_ConnectionCount{};
		PlayerMode m_PlayerMode{};
		ServerMode m_ServerMode{};
	};

	class NetChannelDualFloatLineBase
	{
	public:
		constexpr NetChannelDualFloatLineBase(float f0, float f1) : m_Float0(f0), m_Float1(f1) {}

	protected:
		static bool TryParse(const std::string_view& text, const std::string_view& pattern, float& f0, float& f1);
		void Print(const IConsoleLine::PrintArgs& args, const std::string_view& fmtStr) const;

		float GetFloat0() const { return m_Float0; }
		float GetFloat1() const { return m_Float1; }

	private:
		float m_Float0;
		float m_Float1;
	};

	template<typename TSelf>
	class NetChannelDualFloatLine : public ConsoleLineBase<TSelf>, public NetChannelDualFloatLineBase
	{
		using BaseClass = ConsoleLineBase<TSelf>;

	public:
		static std::shared_ptr<IConsoleLine> TryParse(const std::string_view& text, time_point_t timestamp)
		{
			if (float f0, f1; NetChannelDualFloatLineBase::TryParse(text, TSelf::REGEX_PATTERN, f0, f1))
				return std::make_shared<TSelf>(timestamp, f0, f1);

			return nullptr;
		}

		bool ShouldPrint() const override { return false; }
		void Print(const IConsoleLine::PrintArgs& args) const override final
		{
			return NetChannelDualFloatLineBase::Print(args, TSelf::PRINT_FORMAT_STRING);
		}

	protected:
		NetChannelDualFloatLine(time_point_t timestamp, float f0, float f1) :
			BaseClass(timestamp), NetChannelDualFloatLineBase(f0, f1)
		{
		}
	};

	class NetChannelLatencyLossLine final : public NetChannelDualFloatLine<NetChannelLatencyLossLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetChannelLatencyLossLine(time_point_t timestamp, float latency, float loss) :
			BaseClass(timestamp, latency, loss)
		{
		}

		float GetLatency() const { return GetFloat0(); }
		float GetLoss() const { return GetFloat1(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetChannelLatencyLoss; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- latency: {.1f}, loss {.2f}";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- latency: (\d+\.\d+), loss (\d+\.\d+))regex";
	};

	class NetChannelPacketsLine final : public NetChannelDualFloatLine<NetChannelPacketsLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetChannelPacketsLine(time_point_t timestamp, float inPerSecond, float outPerSecond) :
			BaseClass(timestamp, inPerSecond, outPerSecond)
		{
		}

		float GetInPacketsPerSecond() const { return GetFloat0(); }
		float GetOutPacketsPerSecond() const { return GetFloat1(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetChannelPackets; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- packets: in {.1f}/s, out {.1f}/s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- packets: in (\d+\.\d+)\/s, out (\d+\.\d+)\/s)regex";
	};

	class NetChannelChokeLine final : public NetChannelDualFloatLine<NetChannelChokeLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetChannelChokeLine(time_point_t timestamp, float inPercent, float outPercent) :
			BaseClass(timestamp, inPercent, outPercent) {}

		float GetInPercentChoke() const { return GetFloat0(); }
		float GetOutPercentChoke() const { return GetFloat1(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetChannelChoke; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- choke: in {.2f}, out {.2f}";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- choke: in (\d+\.\d+), out (\d+\.\d+))regex";
	};

	class NetChannelFlowLine final : public NetChannelDualFloatLine<NetChannelFlowLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetChannelFlowLine(time_point_t timestamp, float inKBps, float outKBps) :
			BaseClass(timestamp, inKBps, outKBps)
		{
		}

		float GetInKBps() const { return GetFloat0(); }
		float GetOutKBps() const { return GetFloat1(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetChannelFlow; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- flow: in {.1f}, out {.1f} KB/s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- flow: in (\d+\.\d+), out (\d+\.\d+) kB\/s)regex";
	};

	class NetChannelTotalLine final : public NetChannelDualFloatLine<NetChannelTotalLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetChannelTotalLine(time_point_t timestamp, float inMB, float outMB) :
			BaseClass(timestamp, inMB, outMB)
		{
		}

		float GetInMB() const { return GetFloat0(); }
		float GetOutMB() const { return GetFloat1(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetChannelTotal; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- total: in {.1f}, out {.1f} MB";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- total: in (\d+\.\d+), out (\d+\.\d+) MB)regex";
	};

	class NetLatencyLine final : public NetChannelDualFloatLine<NetLatencyLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetLatencyLine(time_point_t timestamp, float outLatency, float inLatency) :
			BaseClass(timestamp, outLatency, inLatency)
		{
		}

		float GetInLatency() const { return GetFloat1(); }
		float GetOutLatency() const { return GetFloat0(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetLatency; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- Latency: avg out {.2f}s, in {.2f}s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- Latency: avg out (\d+\.\d+)s, in (\d+\.\d+)s)regex";
	};

	class NetLossLine final : public NetChannelDualFloatLine<NetLossLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetLossLine(time_point_t timestamp, float outLoss, float inLoss) :
			BaseClass(timestamp, outLoss, inLoss)
		{
		}

		float GetInLossPercent() const { return GetFloat1(); }
		float GetOutLossPercent() const { return GetFloat0(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetLoss; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- Loss:    avg out {.1f}, in {.1f}";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- Loss:    avg out (\d+\.\d+), in (\d+\.\d+))regex";
	};

	class NetPacketsTotalLine final : public NetChannelDualFloatLine<NetPacketsTotalLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetPacketsTotalLine(time_point_t timestamp, float outPerSecond, float inPerSecond) :
			BaseClass(timestamp, outPerSecond, inPerSecond)
		{
		}

		float GetInPacketsPerSecond() const { return GetFloat1(); }
		float GetOutPacketsPerSecond() const { return GetFloat0(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetPacketsTotal; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- Packets: net total out  {.1f}/s, in {.1f}/s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- Packets: net total out  (\d+\.\d)\/s, in (\d+\.\d)\/s)regex";
	};

	class NetPacketsPerClientLine final : public NetChannelDualFloatLine<NetPacketsPerClientLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetPacketsPerClientLine(time_point_t timestamp, float outPerSecond, float inPerSecond) :
			BaseClass(timestamp, outPerSecond, inPerSecond)
		{
		}

		float GetInPacketsPerSecond() const { return GetFloat1(); }
		float GetOutPacketsPerSecond() const { return GetFloat0(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetPacketsPerClient; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "           per client out {.1f}/s, in {.1f}/s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(           per client out (\d+\.\d)\/s, in (\d+\.\d)\/s)regex";
	};

	class NetDataTotalLine final : public NetChannelDualFloatLine<NetDataTotalLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetDataTotalLine(time_point_t timestamp, float outKBps, float inKBps) :
			BaseClass(timestamp, outKBps, inKBps)
		{
		}

		float GetInKBps() const { return GetFloat1(); }
		float GetOutKBps() const { return GetFloat0(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetDataTotal; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "- Data:    net total out  {.1f}, in {.1f} kB/s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(- Data:    net total out  (\d+\.\d), in (\d+\.\d) kB\/s)regex";
	};

	class NetDataPerClientLine final : public NetChannelDualFloatLine<NetDataPerClientLine>
	{
		using BaseClass = NetChannelDualFloatLine;

	public:
		NetDataPerClientLine(time_point_t timestamp, float outKBps, float inKBps) :
			BaseClass(timestamp, outKBps, inKBps)
		{
		}

		float GetInKBps() const { return GetFloat1(); }
		float GetOutKBps() const { return GetFloat0(); }

		ConsoleLineType GetType() const override { return ConsoleLineType::NetDataPerClient; }

		static constexpr std::string_view PRINT_FORMAT_STRING =  "           per client out {.1f}, in {.1f} kB/s";
		static constexpr std::string_view REGEX_PATTERN = R"regex(           per client out (\d+\.\d), in (\d+\.\d) kB\/s)regex";
	};
}
