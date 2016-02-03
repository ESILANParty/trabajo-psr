/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <ns3/packet.h>
#include "ns3/average.h"
#include "ns3/ethernet-header.h"

using namespace ns3;


class Observador
{
public:
  Observador  ();
  void      MacTxClient (Ptr<const Packet> packet);
  void      MacRxClient (Ptr<const Packet> packet);
  void      MacTxServ (Ptr<const Packet> packet);
  void      MacRxServ (Ptr<const Packet> packet);
  double    TiempoDesdeClientMedio ();
  double    TiempoDesdeServMedio ();
  double    PorcentajeCorrectos ();
  bool      EsPqtIP (Ptr<Packet> packet);


private:
  Average<double> m_avgTiemposDesdeClient;
  std::map<uint64_t, Time> m_tiempoDesdeClient;
  Average<double> m_avgTiemposDesdeServ;
  std::map<uint64_t, Time> m_tiempoDesdeServ;
  EthernetHeader m_cabeceraEth;
  int             m_totalPaquetesTx;
  int             m_totalPaquetesRx;
};
