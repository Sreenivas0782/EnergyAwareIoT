#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/energy-module.h"

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("EEIoTWPAN");

/*
 * =========================================================
 * Energy Aware Agent
 * =========================================================
 */

class EnergyAwareAgent
{
public:

    double qValue;

    EnergyAwareAgent()
    {
        qValue = 0.5;
    }

    double
    ComputeReward(double energy,
                  double throughput,
                  double delay)
    {
        return (0.5 * throughput)
             + (0.3 * energy)
             - (0.2 * delay);
    }

    void
    UpdateQ(double reward)
    {
        double alpha = 0.1;

        qValue =
            qValue + alpha * (reward - qValue);
    }
};

std::vector<EnergyAwareAgent> agents;

/*
 * =========================================================
 * RL Learning Scheduler
 * =========================================================
 */

void
RunLearning(EnergySourceContainer sources)
{
    std::cout
        << "\n========== RL STATUS ==========\n";

    for (uint32_t i = 0; i < sources.GetN(); i++)
    {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(
                sources.Get(i));

        double energy =
            src->GetRemainingEnergy();

        double normalizedEnergy =
            energy / 100.0;

        double throughput =
            0.5 + ((double) rand() / RAND_MAX);

        double delay =
            0.1 + ((double) rand() / RAND_MAX);

        double reward =
            agents[i].ComputeReward(
                normalizedEnergy,
                throughput,
                delay);

        agents[i].UpdateQ(reward);

        std::cout
            << "Node "
            << i
            << " Energy="
            << energy
            << " Reward="
            << reward
            << " Q="
            << agents[i].qValue
            << std::endl;
    }

    Simulator::Schedule(
        Seconds(5.0),
        &RunLearning,
        sources);
}

/*
 * =========================================================
 * Main
 * =========================================================
 */

int
main(int argc, char *argv[])
{
    uint32_t numNodes = 10;

    double simulationTime = 30.0;

    CommandLine cmd;

    cmd.AddValue(
        "numNodes",
        "Number of WPAN Nodes",
        numNodes);

    cmd.Parse(argc, argv);

    /*
     * =====================================================
     * Create Nodes
     * =====================================================
     */

    NodeContainer nodes;

    nodes.Create(numNodes);

    agents.resize(numNodes);

    /*
     * =====================================================
     * Mobility
     * =====================================================
     */

    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(10.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(5),
        "LayoutType",
        StringValue("RowFirst"));

    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    mobility.Install(nodes);

    /*
     * =====================================================
     * LR-WPAN
     * =====================================================
     */

    LrWpanHelper lrWpanHelper;

    NetDeviceContainer devices =
        lrWpanHelper.Install(nodes);

    /*
     * VERY IMPORTANT
     * Assign UNIQUE MAC16 addresses
     * to avoid IPv6 collisions
     */

    for (uint32_t i = 0; i < devices.GetN(); i++)
    {
        Ptr<lrwpan::LrWpanNetDevice> dev =
            DynamicCast<lrwpan::LrWpanNetDevice>(
                devices.Get(i));

        dev->GetMac()->SetPanId(0x1234);

        dev->GetMac()->SetShortAddress(
            Mac16Address::Allocate());
    }

    /*
     * =====================================================
     * 6LoWPAN
     * =====================================================
     */

    SixLowPanHelper sixlowpan;

    NetDeviceContainer sixDevices =
        sixlowpan.Install(devices);

    /*
     * =====================================================
     * Internet Stack
     * =====================================================
     */

    InternetStackHelper internet;

    internet.Install(nodes);

    /*
     * =====================================================
     * IPv6 Addressing
     * =====================================================
     */

    Ipv6AddressHelper ipv6;

    ipv6.SetBase(
        Ipv6Address("2001:1::"),
        Ipv6Prefix(64));

    Ipv6InterfaceContainer interfaces =
        ipv6.Assign(sixDevices);

    /*
     * =====================================================
     * Energy Model
     * =====================================================
     */

    BasicEnergySourceHelper energyHelper;

    energyHelper.Set(
        "BasicEnergySourceInitialEnergyJ",
        DoubleValue(100.0));

    EnergySourceContainer sources =
        energyHelper.Install(nodes);

    /*
     * =====================================================
     * UDP SERVER
     * =====================================================
     */

    uint16_t port = 9;

    UdpServerHelper server(port);

    ApplicationContainer serverApps =
        server.Install(
            nodes.Get(numNodes - 1));

    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(simulationTime));

    /*
     * =====================================================
     * UDP CLIENT
     * =====================================================
     */

    UdpClientHelper client(
        interfaces.GetAddress(numNodes - 1, 1),
        port);

    client.SetAttribute(
        "MaxPackets",
        UintegerValue(1000));

    client.SetAttribute(
        "Interval",
        TimeValue(Seconds(1.0)));

    client.SetAttribute(
        "PacketSize",
        UintegerValue(64));

    ApplicationContainer clientApps =
        client.Install(nodes.Get(0));

    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simulationTime));

    /*
     * =====================================================
     * FLOW MONITOR
     * =====================================================
     */

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();

    /*
     * =====================================================
     * RL Scheduler
     * =====================================================
     */

    Simulator::Schedule(
        Seconds(5.0),
        &RunLearning,
        sources);

    /*
     * =====================================================
     * Run Simulation
     * =====================================================
     */

    Simulator::Stop(
        Seconds(simulationTime));

    Simulator::Run();

    /*
     * =====================================================
     * FLOW RESULTS
     * =====================================================
     */

    monitor->CheckForLostPackets();

    std::map<FlowId,
             FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    std::cout
        << "\n========== FLOW RESULTS ==========\n";

    for (auto const &i : stats)
    {
        std::cout
            << "\nFlow ID: "
            << i.first
            << std::endl;

        std::cout
            << "Tx Packets = "
            << i.second.txPackets
            << std::endl;

        std::cout
            << "Rx Packets = "
            << i.second.rxPackets
            << std::endl;

        double pdr = 0.0;

        if (i.second.txPackets > 0)
        {
            pdr =
                ((double)i.second.rxPackets /
                 i.second.txPackets) * 100.0;
        }

        std::cout
            << "Packet Delivery Ratio = "
            << pdr
            << " %"
            << std::endl;

        double throughput =
            i.second.rxBytes * 8.0 /
            simulationTime /
            1024;

        std::cout
            << "Throughput = "
            << throughput
            << " Kbps"
            << std::endl;

        if (i.second.rxPackets > 0)
        {
            double delay =
                i.second.delaySum.GetSeconds() /
                i.second.rxPackets;

            std::cout
                << "Average Delay = "
                << delay
                << " sec"
                << std::endl;
        }
    }

    /*
     * =====================================================
     * FINAL ENERGY STATUS
     * =====================================================
     */

    std::cout
        << "\n========== FINAL ENERGY ==========\n";

    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(
                sources.Get(i));

        std::cout
            << "Node "
            << i
            << " Remaining Energy = "
            << src->GetRemainingEnergy()
            << " J"
            << std::endl;
    }

    /*
     * =====================================================
     * Destroy Simulator
     * =====================================================
     */

    Simulator::Destroy();

    return 0;
}
