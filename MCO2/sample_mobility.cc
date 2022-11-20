MobilityHelper mobility;

ObjectFactory pos;

pos.SetTypeId (
"ns3::RandomRectanglePositionAllocator");

pos.Set (
"X", RandomVariableValue (UniformVariable (0.0, m_gridSize)));

pos.Set (
"Y", RandomVariableValue (UniformVariable (0.0, m_gridSize)));

Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();

mobility.SetMobilityModel (
    "ns3::RandomWaypointMobilityModel", 
    "Speed", 
    RandomVariableValue (ConstantVariable (1200)), 
    "Pause", 
    RandomVariableValue (ConstantVariable (0)), 
    "PositionAllocator", 
    PointerValue (taPositionAlloc));

mobility.SetPositionAllocator (taPositionAlloc);

mobility.Install (tas);