#include <iostream>

#include <cosmos/error/ApiError.hxx>
#include <cosmos/main.hxx>
#include <cosmos/net/ifs/InterfaceEnumerator.hxx>
#include <cosmos/net/packet/aux.hxx>
#include <cosmos/net/packet/PacketSocket.hxx>

class NetDump :
		public cosmos::MainContainerArgsNoRetval {
protected:

	void main(const std::string_view argv0, const cosmos::StringViewVector &args) override {
		if (args.size() > 1) {
			std::cerr << argv0 << " [network-interface]\n";
			throw cosmos::ExitStatus::FAILURE;
		} else if (!args.empty()) {
			lookupIndex(args[0]);
		}

		try {
			cosmos::CookedPacketSocket socket{cosmos::EthernetProtocol::ALL};
			bindToIF(socket);
			capture(socket);
		} catch (const cosmos::ApiError &err) {
			std::cerr << "error: " << err.what() << "\n";
			throw cosmos::ExitStatus::FAILURE;
		}
	};

	void lookupIndex(const std::string_view name) {
		cosmos::InterfaceEnumerator enumerator{true};
		for (const auto &info : enumerator) {
			if (info.name() == name) {
				m_dump_if = info.index();
			}
		}

		if (m_dump_if == cosmos::InterfaceIndex::ANY) {
			std::cerr << name << ": unknown network interface\n";
			throw cosmos::ExitStatus::FAILURE;
		}

		std::cout << "Capturing packets on '" << name << "'\n";
	}

	void capture(cosmos::PacketSocket &sock) {
		std::vector<char> buf;
		buf.resize(1024);
		cosmos::ReceiveMessageHeader header;
		header.setControlBufferSize(1024);
		cosmos::PacketAuxDataMessage aux_msg;

		while (true) {
			header.iovec.clear();
			header.iovec.push_back(cosmos::InputMemoryRegion{buf.data(), buf.size()});
			auto addr = sock.receiveMessageFrom(header);
			if (!addr) {
				std::cerr << "received packet from unknown source\n";
				continue;
			}

			const auto &mac = addr->macAddress();

			std::cout << std::format("{} bytes from {:x}:{:x}:{:x}:{:x}:{:x}:{:x} protocol {:#x} type {}",
				header.iovec[0].getLength(),
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
				cosmos::to_integral(addr->protocol()),
				typeLabel(addr->packetType())
			) << std::endl;

			for (const auto &ctrl: header) {
				if (!aux_msg.matches(ctrl))
					continue;

				aux_msg.deserialize(ctrl);

				const auto &data = aux_msg.data();
				std::cout <<
					std::format("\taux data: size = {}, capture = {}, mac_off={}, net_off={}, has_vlan_tci={}, has_vlan_tpid={}",
							data.packetSize(),
							data.capturedSize(),
							data.macOffset(),
							data.netOffset(),
							data.hasVLANTag(),
							data.hasVLANTagProtocol())
					<< std::endl;
			}
		}
	}

	const char* typeLabel(const cosmos::PacketType type) {
		using enum cosmos::PacketType;
		switch (type) {
			case HOST: return "HOST";
			case BROADCAST: return "BROADCAST";
			case MULTICAST: return "MULTICAST";
			case OTHERHOST: return "OTHERHOST";
			case OUTGOING: return "OUTGOING";
			default: return "???";
		}
	}

	void bindToIF(cosmos::PacketSocket &socket) {
		cosmos::LinkLayerAddress addr;
		addr.setIfindex(m_dump_if);
		addr.setProtocol(cosmos::EthernetProtocol::ALL);
		socket.bind(addr);

		auto options = socket.packetOptions();

		if (m_dump_if != cosmos::InterfaceIndex::ANY) {
			options.addMembership(
					cosmos::PacketOptions::MembershipReq{m_dump_if}.setPromiscuous());
		}

		options.enableAuxData(true);
	}

	cosmos::InterfaceIndex m_dump_if = cosmos::InterfaceIndex::ANY;
};

int main(const int argc, const char **argv) {
	cosmos::main<NetDump>(argc, argv);
}
