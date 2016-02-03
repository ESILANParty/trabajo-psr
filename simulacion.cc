/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-rate.h"
#include "ns3/gnuplot.h"
#include "Observador.h"

using namespace ns3;
#define ALEATORIO 0
#define SECUENCIAL 1
#define CURVAS 1
#define VALORES_USUARIOS 10
#define PASO_USUARIOS 50
#define REPETICIONES 1

NS_LOG_COMPONENT_DEFINE ("fuentes12");

Observador simulacion (double tasaMediaIn, double tasaMediaOut, uint32_t tamPaqueteIn,
  uint32_t tamPaqueteOut, uint32_t usuariosXbus, uint32_t numRouters,
  uint32_t numServidores, double retardo, uint16_t modoSeleccion);

int seleccionaServidor(uint32_t numServidores, uint16_t modoSeleccion, double * servidor);

int
main (int argc, char *argv[])
{
  GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));
  Time::SetResolution (Time::NS);

  // Intervalos de tiempo en segundos
  double intervaloMedioUsuario = 0.01;
  double intervaloMedioServidor = 0.005;
  // Paquetes en Bytes
  uint32_t tamPaqueteIn = 136;
  uint32_t tamPaqueteOut = 71;
  uint32_t usuariosXbus = 1;
  uint32_t numRouters = 100;
  uint32_t numServidores = 1;
  double retardo = 2;
  uint16_t modoSeleccion = SECUENCIAL;

  // Parametros por linea de comandos
  CommandLine cmd;
  cmd.AddValue ("intervaloMedioUsuario", "Tasa media de entrada en los equipos", intervaloMedioUsuario);
  cmd.AddValue ("intervaloMedioServidor", "Tasa media de salida en los equipos", intervaloMedioServidor);
  cmd.AddValue ("tamPaqueteIn", "Tamaño de paquetes de entrada", tamPaqueteIn);
  cmd.AddValue ("tamPaqueteOut", "Tamaño de paquetes de salida", tamPaqueteOut);
  cmd.AddValue ("usuariosXbus", "Equipos de jugadores por cada bus CSMA", usuariosXbus);
  cmd.AddValue ("numRouters", "Numero de routers", numRouters);
  cmd.AddValue ("numServidores", "Numero de servidores", numServidores);
  cmd.AddValue ("retardo", "Retardo de envio", retardo);
  cmd.AddValue ("modoSelección", "Modo de selección del servidor", modoSeleccion);
  cmd.Parse (argc,argv);

  // Objetos necesarios para las graficas
  Gnuplot plotPqtsCorrectos;
  Gnuplot plotLatencia;
  Gnuplot2dDataset datosPqts[CURVAS];
  Gnuplot2dDataset datosLatencia[CURVAS];
  double ts13n95p = 2.1788; // n = 13, 95% -> (12, 0.025)
  double z_PqtsCorrectos;
  Average<double> avgPorcentajesPqtsCorrectos;
  Average<double> avgLatenciasMedias;

  // Valores precargados para las simulaciones
  std::ostringstream tituloPqtsCorrectos;
  tituloPqtsCorrectos << "Porcentaje de paquetes correctos";
  std::ostringstream tituloLatencia;
  tituloLatencia << "Latencia";
  plotPqtsCorrectos.SetTitle (tituloPqtsCorrectos.str ());
  plotPqtsCorrectos.SetLegend ("Números de usuarios", "Paquetes correctos");
  plotLatencia.SetTitle (tituloLatencia.str ());
  plotLatencia.SetLegend ("Números de usuarios", "Latencia (ms)");

  // Bucle para los distintos valores de servidores
	for(int i=0; i<CURVAS; i++)
	{
    // Configuracion de las curvas i de cada numero de servidores
    std::ostringstream rotulo;
    rotulo << "nº de servidores: " << (i + 1);
    datosPqts[i] = Gnuplot2dDataset (rotulo.str ());
    datosPqts[i].SetStyle(Gnuplot2dDataset::LINES_POINTS);
	  datosPqts[i].SetErrorBars(Gnuplot2dDataset::Y);
    datosLatencia[i] = Gnuplot2dDataset (rotulo.str ());
    datosLatencia[i].SetStyle(Gnuplot2dDataset::LINES_POINTS);
    //datosLatencia[i].SetErrorBars(GnuplotLatenciadDataset::Y);

    // Bucle para los distintos tiempos medios del estado On de las fuentes
    for(int n=0; n<VALORES_USUARIOS; n++)
    {
      NS_LOG_INFO ("Simulando con nº de usuarios: " << (n*PASO_USUARIOS+PASO_USUARIOS) << " y nº servidores: " << (i+1));
      // Bucle de simulaciones con los mismos parametros
      for(int k=0; k<REPETICIONES; k++)
      {
        // Hacemos una simulacion simple
        Observador observador = simulacion (intervaloMedioUsuario, intervaloMedioServidor, tamPaqueteIn,
          tamPaqueteOut, usuariosXbus, (n*PASO_USUARIOS+PASO_USUARIOS), (i+1), retardo, modoSeleccion);

        avgPorcentajesPqtsCorrectos.Update (observador.PorcentajeCorrectos ());
        avgLatenciasMedias.Update (observador.TiempoDesdeClientMedio ()
        + observador.TiempoDesdeServMedio ());
      }

      // Calculamos el intervalo de confianza del 95%
      z_PqtsCorrectos = ts13n95p*sqrt(avgPorcentajesPqtsCorrectos.Var () / REPETICIONES);
      NS_LOG_INFO ("IC95% de % paquetes correctos: ("
                    << avgPorcentajesPqtsCorrectos.Avg()-z_PqtsCorrectos << ", "
                    << avgPorcentajesPqtsCorrectos.Avg()+z_PqtsCorrectos << ")");

      // y aÃ±adimos los datos
      datosPqts[i].Add(n*PASO_USUARIOS+PASO_USUARIOS, avgPorcentajesPqtsCorrectos.Avg(), z_PqtsCorrectos);
      datosLatencia[i].Add(n*PASO_USUARIOS+PASO_USUARIOS, avgLatenciasMedias.Avg());

      // y reseteamos los acumuladores
      avgPorcentajesPqtsCorrectos.Reset ();
      avgLatenciasMedias.Reset ();
    }

    // AÃ±adimos los datos a las graficas
    plotPqtsCorrectos.AddDataset(datosPqts[i]);
  	plotLatencia.AddDataset(datosLatencia[i]);
  }

  // Guardamos las graficas en los ficheros pedidos
  std::ofstream fichero1("paquetesCorrectos.plt");
  plotPqtsCorrectos.GenerateOutput(fichero1);
  fichero1 << "pause -1" << std::endl;
  fichero1.close();
  std::ofstream fichero2("latencia.plt");
  plotLatencia.GenerateOutput(fichero2);
  fichero2 << "pause -1" << std::endl;
  fichero2.close();

  return 0;
}

Observador simulacion (double intervaloMedioUsuario, double intervaloMedioServidor, uint32_t tamPaqueteIn,
  uint32_t tamPaqueteOut, uint32_t usuariosXbus, uint32_t numRouters,
  uint32_t numServidores, double retardo, uint16_t modoSeleccion)
{
  // Variables aleatorias del sistema
  Ptr<NormalRandomVariable> IntervaloUsuario = CreateObject<NormalRandomVariable> ();
  Ptr<NormalRandomVariable> IntervaloServidor = CreateObject<NormalRandomVariable> ();
  IntervaloUsuario->SetAttribute ("Mean", DoubleValue(intervaloMedioUsuario));
  IntervaloServidor->SetAttribute ("Mean", DoubleValue(intervaloMedioServidor));


  // Variable para calcular los estadisticos
  Observador observador;

  // Creacion de los nodos p2p (servidores y gateway)
  NS_LOG_INFO("Creando los nodos p2p de los servidores...");
  NodeContainer p2pServNodes;
  p2pServNodes.Create (numServidores+1);

  // Creacion de los nodos p2p de todos los routers
  NS_LOG_INFO("Creando los nodos p2p de todos los routers...");
  NodeContainer p2pRoutersNodes;
  p2pRoutersNodes.Add (p2pServNodes.Get (0));
  p2pRoutersNodes.Create (numRouters);

  // Creacion de los nodos p2p de cada router con x equipos
  NS_LOG_INFO("Creando los nodos p2p de cada router con x equipos...");
  NodeContainer p2pUsuariosNodes [numRouters];
  for (uint32_t i=0; i<numRouters; i++)
  {
    p2pUsuariosNodes[i].Add (p2pRoutersNodes.Get (i+1));
    p2pUsuariosNodes[i].Create (usuariosXbus);
  }

  // Instalacion de los servidores a los P2P con el router
  // El nodo 0 es el router y el resto los servidores conectados a el
  NS_LOG_INFO("Instalando las conexiones p2p de los servidores...");
  PointToPointHelper pointToPointServ [numServidores];
  NetDeviceContainer p2pServDevices [numServidores];
  for(uint32_t i=0; i<numServidores; i++)
  {
    pointToPointServ[i].SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    pointToPointServ[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
    p2pServDevices[i] = pointToPointServ[i].Install (p2pServNodes.Get (0), p2pServNodes.Get (i+1));
  }

  // Instalacion de las conexiones p2p de los routers
  NS_LOG_INFO("Instalando las conexiones p2p de los routers...");
  PointToPointHelper pointToPointRouters [numRouters];
  NetDeviceContainer p2pRoutersDevices [numRouters];
  for(uint32_t i=0; i<numRouters; i++)
  {
    pointToPointRouters[i].SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
    pointToPointRouters[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
    p2pRoutersDevices[i] = pointToPointRouters[i].Install (p2pRoutersNodes.Get (0), p2pRoutersNodes.Get (i+1));
  }

  // Instalacion de las conexiones p2p de los routers con los equipos
  NS_LOG_INFO("Instalando las conexiones p2p de los routers con los equipos...");
  PointToPointHelper pointToPointUsuarios [numRouters*usuariosXbus];
  NetDeviceContainer p2pUsuariosDevices [numRouters];
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      pointToPointUsuarios[i].SetDeviceAttribute ("DataRate", StringValue ("10Gbps"));
      pointToPointUsuarios[i].SetChannelAttribute ("Delay", StringValue ("1ms"));
      p2pUsuariosDevices[i].Add (pointToPointUsuarios[i*usuariosXbus+j].Install (p2pUsuariosNodes[i].Get (0), p2pUsuariosNodes[i].Get (j+1)));
    }
  }
  //Capturador de envío y recepción de los equipos usuarios
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      p2pUsuariosDevices[i].Get (j+1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacTx", MakeCallback(&Observador::MacTxClient, &observador));
      p2pUsuariosDevices[i].Get (j+1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacRx", MakeCallback(&Observador::MacRxClient, &observador));
    }
  }
  // Capturador de envío y recepción de los servidores
  for(uint32_t i=0; i<numServidores; i++)
  {
      p2pServDevices[i].Get (1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacTx", MakeCallback(&Observador::MacTxServ, &observador));
      p2pServDevices[i].Get (1)->GetObject<PointToPointNetDevice> ()
      ->TraceConnectWithoutContext ("MacRx", MakeCallback(&Observador::MacRxServ, &observador));
  }

  // Instalamos la pila TCP/IP en todos los nodos
  NS_LOG_INFO("Instalando la pila TCP/IP en todos los nodos...");
  InternetStackHelper stack;
  for(uint32_t i=1; i<=numServidores; i++)
  {
    stack.Install (p2pServNodes.Get (i));
  }
  stack.Install (p2pRoutersNodes);
  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=1; j<=usuariosXbus; j++)
    {
      stack.Install (p2pUsuariosNodes[i].Get (j));
    }
  }

  // Asignamos direcciones a cada una de las interfaces
  NS_LOG_INFO("Asignando direcciones IP,");
  NS_LOG_INFO("a los nodos p2p...");
  Ipv4AddressHelper address;
  Ipv4InterfaceContainer p2pServInterfaces [numServidores];
  for(uint32_t i=0; i<numServidores; i++)
  {
    std::ostringstream network;
    network << "10.1." << (i+1) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pServInterfaces[i] = address.Assign (p2pServDevices[i]);
  }


  NS_LOG_INFO("a los nodos p2p encargados de hacer de gateways...");
  Ipv4InterfaceContainer p2pRoutersInterfaces;
  uint32_t itotal=numServidores+1;
  uint32_t subred =1;
  for(uint32_t i=0; i<numRouters; i++)
  {
  	if(itotal<250){
    	itotal= itotal+1;
    }else{
    	itotal=1;
    	subred= subred+1;
    }
    std::ostringstream network;
    network << "10."<< subred <<"." << itotal << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pRoutersInterfaces = address.Assign (p2pRoutersDevices[i]);
  }

  NS_LOG_INFO("y a los nodos p2p de los equipos usuario...");
  Ipv4InterfaceContainer p2pUsuariosInterfaces [numRouters];
  for(uint32_t i=0; i<numRouters; i++)
  {
    if(itotal<250){
    	itotal= itotal+1;
    }else{
    	itotal=1;
    	subred= subred+1;
    }
    std::ostringstream network;
    network << "10."<< subred <<"." << (itotal) << ".0";
    address.SetBase (network.str ().c_str (), "255.255.255.0");
    p2pUsuariosInterfaces[i] = address.Assign (p2pUsuariosDevices[i]);
  }

  // Calculamos las rutas del escenario. Con este comando, los
  //     nodos de la red de área local definen que para acceder
  //     al nodo del otro extremo del enlace punto a punto deben
  //     utilizar el primer nodo como ruta por defecto.
  NS_LOG_INFO("Calculando rutas del escenario...");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Creamos las aplicaciones serveridoras UDP en los nodos servidores y en los nodos de los jugadores
  NS_LOG_INFO("Creando las aplicaciones servidor UDP");
  uint16_t port = 6000;
  UdpServerHelper server (port);
  NodeContainer serverNodes;
  for(uint32_t i=1; i<=numServidores; i++)
  {
    serverNodes.Add (p2pServNodes.Get (i));
  }

  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=1; j<=usuariosXbus; j++)
    {
     serverNodes.Add (p2pUsuariosNodes[i].Get (j));
    }
  }
  ApplicationContainer serverApps = server.Install (serverNodes);
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (3.0));

  // Creamos las aplicaciones clientes UDP en los nodos clientes eligiendo el servidor de forma aleatoria...
  NS_LOG_INFO("Creando las aplicaciones cliente UDP");
  Time interPacketInterval = Seconds (0.01);   //... y las aplicaciones clientes en los servidores que han sido elegidos
  uint32_t maxPacketCount = 10000;
  UdpClientHelper client[numRouters*usuariosXbus];
  ApplicationContainer clientApps;
  UdpClientHelper serv[numRouters*usuariosXbus];
  ApplicationContainer pingApps;

  for(uint32_t i=0; i<numRouters; i++)
  {
    for(uint32_t j=0; j<usuariosXbus; j++)
    {
      double servidor = 0;
      servidor = seleccionaServidor(numServidores, modoSeleccion, &servidor);
      client[i*usuariosXbus+j] = UdpClientHelper (p2pServInterfaces[(int)servidor].GetAddress (1), port);
      client[i*usuariosXbus+j].SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client[i*usuariosXbus+j].SetAttribute ("Interval", TimeValue (interPacketInterval));
      client[i*usuariosXbus+j].SetAttribute ("PacketSize", UintegerValue (tamPaqueteOut));
      clientApps.Add (client[i*usuariosXbus+j].Install (p2pUsuariosNodes[i].Get (j+1)));

      //Ya Conocemos el servidor elegido y el equipo que lo elige, instalamos el cliente en dicho servidor
      //poniendo como servAddress la dirección del equipo que lo ha elegido.
      serv[i*usuariosXbus+j] = UdpClientHelper (p2pUsuariosInterfaces[i].GetAddress (j+1), port);
      serv[i*usuariosXbus+j].SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      serv[i*usuariosXbus+j].SetAttribute ("Interval", TimeValue (interPacketInterval));
      serv[i*usuariosXbus+j].SetAttribute ("PacketSize", UintegerValue (tamPaqueteIn));
      clientApps.Add (serv[i*usuariosXbus+j].Install (p2pServNodes.Get ((int)servidor+1)));
    }
  }
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (3.0));

  pointToPointUsuarios[0].EnablePcap ("trabajo-psr", p2pUsuariosDevices[0].Get (1), true);
  pointToPointServ[0].EnablePcap ("trabajo-psr-Serv", p2pServDevices[0].Get (1), true);

  // Lanzamos la simulación
  NS_LOG_INFO("Simulando...");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO("¡Hecho!");

  return observador;
}

//Función que devuelve un número aleatorio que será el servidor asignado.
int seleccionaServidor(uint32_t numServidores, uint16_t modoSeleccion, double * servidor)
{

  if(modoSeleccion == ALEATORIO)
  {
    double min = 0.0;
    double max = numServidores;
    Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();           //el máximo está fuera del intervalo que devuelve.
    x->SetAttribute ("Min", DoubleValue (min));
    x->SetAttribute ("Max", DoubleValue (max));
    *servidor = x->GetValue ();
   }
   else
   {
    if(*servidor == numServidores-1 || *servidor == 0)
      *servidor=0;
    else
      *servidor= (*servidor)+1;
   }

  return (int) *servidor;
}
