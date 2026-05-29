#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/energy-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;
using namespace ns3::energy;

NS_LOG_COMPONENT_DEFINE("EEIoT");

class EnergyAwareAgent
{
public:

    double qValue;
    double trust;

    EnergyAwareAgent()
    {
        qValue = 0.5;
        trust = 1.0;
    }

    double ComputeReward(double energy,
                         double delay,
                         double throughput)
    {
        return (0.5 * throughput)
             + (0.3 * energy)
             - (0.2 * delay);
    }

    void UpdateQ(double reward)
    {
        double alpha = 0.1;

        qValue =
            qValue + alpha * (reward - qValue);
    }

    double GetMetric(double residualEnergy)
    {
        return qValue *
               trust *
               residualEnergy;
    }
};

std::vector<EnergyAwareAgent> agents;

void
PrintNodeEnergy(NodeContainer nodes,
                EnergySourceContainer sources)
{
    std::cout << "\n ENERGY STATUS ==========\n";

    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(
                sources.Get(i));

        double remaining =
            src->GetRemainingEnergy();

        std::cout
            << "Node "
            << i
            << " Remaining Energy = "
            << remaining
            << " J"
            << std::endl;
    }

    Simulator::Schedule(
        Seconds(5.0),
        &PrintNodeEnergy,
        nodes,
        sources);
}

void
RunLearning(EnergySourceContainer sources)
{
    for (uint32_t i = 0; i < sources.GetN(); i++)
    {
        Ptr<BasicEnergySource> src =
            DynamicCast<BasicEnergySource>(
                sources.Get(i));

        double energy =
            src->GetRemainingEnergy();

        double normalizedEnergy =
            energy / 100.0;

        double delay =
            0.1 + ((double) rand() / RAND_MAX);

        double throughput =
            0.5 + ((double) rand() / RAND_MAX);

        double reward =
            agents[i].ComputeReward(
                normalizedEnergy,
                delay,
                throughput);

        agents[i].UpdateQ(reward);

        double metric =
            agents[i].GetMetric(
                normalizedEnergy);

        std::cout
            << "Node "
            << i
            << " Reward = "
            << reward
            << " Q = "
            << agents[i].qValue
            << " Metric = "
            << metric
            << std::endl;
    }

    Simulator::Schedule(
        Seconds(2.0),
        &RunLearning,
        sources);
}

int
main(int argc, char *argv[])
{
    uint32_t numNodes = 20;

    double simulationTime = 60.0;

    CommandLine cmd;

    cmd.AddValue(
        "numNodes",
        "Number of IoT Nodes",
        numNodes);

    cmd.Parse(argc, argv);

    NodeContainer nodes;

    nodes.Create(numNodes);

    agents.resize(numNodes);

    /*
     * WiFi Configuration
     */

    WifiHelper wifi;

    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;

    phy.SetChannel(channel.Create());

    WifiMacHelper mac;

    mac.SetType("ns3::AdhocWifiMac");

    NetDeviceContainer devices =
        wifi.Install(phy, mac, nodes);

    /*
     * Mobility Configuration
     * ConstantPositionMobilityModel
     * because IoT nodes are stationary
     */

    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(20.0),
        "DeltaY", DoubleValue(20.0),
        "GridWidth", UintegerValue(5),
        "LayoutType",
        StringValue("RowFirst"));

    mobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");

    mobility.Install(nodes);

    /*
     * Internet Stack
     */

    InternetStackHelper internet;

    internet.Install(nodes);

    Ipv4AddressHelper address;

    address.SetBase(
        "10.1.1.0",
        "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(devices);

    /*
     * Energy Model
     */

    BasicEnergySourceHelper energyHelper;

    energyHelper.Set(
        "BasicEnergySourceInitialEnergyJ",
        DoubleValue(100.0));

    EnergySourceContainer sources =
        energyHelper.Install(nodes);

    WifiRadioEnergyModelHelper radioHelper;

    radioHelper.Set(
        "TxCurrentA",
        DoubleValue(0.0174));

    radioHelper.Set(
        "RxCurrentA",
        DoubleValue(0.0197));

    radioHelper.Install(
        devices,
        sources);

    /*
     * Application Layer
     */

    uint16_t port = 9;

    OnOffHelper onoff(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            interfaces.GetAddress(numNodes - 1),
            port));

    onoff.SetConstantRate(
        DataRate("256kbps"));

    ApplicationContainer senderApp =
        onoff.Install(nodes.Get(0));

    senderApp.Start(Seconds(1.0));
    senderApp.Stop(Seconds(simulationTime));

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(
            Ipv4Address::GetAny(),
            port));

    ApplicationContainer receiverApp =
        sink.Install(nodes.Get(numNodes - 1));

    receiverApp.Start(Seconds(0.0));
    receiverApp.Stop(Seconds(simulationTime));

    /*
     * Flow Monitor
     */

    FlowMonitorHelper flowHelper;

    Ptr<FlowMonitor> monitor =
        flowHelper.InstallAll();

    /*
     * RL Scheduling
     */

    Simulator::Schedule(
        Seconds(2.0),
        &RunLearning,
        sources);

    Simulator::Schedule(
        Seconds(5.0),
        &PrintNodeEnergy,
        nodes,
        sources);

    /*
     * Run Simulation
     */

    Simulator::Stop(
        Seconds(simulationTime));

    Simulator::Run();

    /*
     * Results
     */

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            flowHelper.GetClassifier());

    std::map<FlowId,
             FlowMonitor::FlowStats> stats =
        monitor->GetFlowStats();

    std::cout
        << "\n========== FLOW RESULTS ==========\n";

    for (auto const &i : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
            classifier->FindFlow(i.first);

        std::cout
            << "\nFlow ID: "
            << i.first
            << std::endl;

        std::cout
            << "Source Address: "
            << t.sourceAddress
            << std::endl;

        std::cout
            << "Destination Address: "
            << t.destinationAddress
            << std::endl;

        std::cout
            << "Tx Packets = "
            << i.second.txPackets
            << std::endl;

        std::cout
            << "Rx Packets = "
            << i.second.rxPackets
            << std::endl;

        double pdr =
            ((double)i.second.rxPackets /
             i.second.txPackets) * 100.0;

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
     * Final Energy
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
            << " Final Energy = "
            << src->GetRemainingEnergy()
            << " J"
            << std::endl;
    }

    Simulator::Destroy();

    return 0;
}
