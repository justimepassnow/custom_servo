# Graph Report - C:\Users\AKSHAY\Desktop\code\projects\custom servo  (2026-05-17)

## Corpus Check
- Corpus is ~9,748 words - fits in a single context window. You may not need a graph.

## Summary
- 25 nodes · 19 edges · 12 communities (4 shown, 8 thin omitted)
- Extraction: 37% EXTRACTED · 63% INFERRED · 0% AMBIGUOUS · INFERRED: 12 edges (avg confidence: 0.88)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Smart Servo Overview|Smart Servo Overview]]
- [[_COMMUNITY_CH32V003 Integration|CH32V003 Integration]]
- [[_COMMUNITY_BDR612H Motor Driver|BDR612H Motor Driver]]
- [[_COMMUNITY_Signal Filtering|Signal Filtering]]
- [[_COMMUNITY_Programming  Flashing|Programming / Flashing]]
- [[_COMMUNITY_Analog Current Sensing|Analog Current Sensing]]
- [[_COMMUNITY_TVS Protection|TVS Protection]]
- [[_COMMUNITY_Target Application|Target Application]]
- [[_COMMUNITY_Star Grounding Topology|Star Grounding Topology]]
- [[_COMMUNITY_Direct Memory Access|Direct Memory Access]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 11|Community 11]]

## God Nodes (most connected - your core abstractions)
1. `CH32V003` - 10 edges
2. `Smart Servo Conversion Project` - 3 edges
3. `RC Low-Pass Filter` - 3 edges
4. `BDR612H` - 3 edges
5. `PY32F002A` - 2 edges
6. `PicoRVD` - 2 edges
7. `1kHz PID Loop` - 2 edges
8. `Star Grounding Topology` - 1 edges
9. `ch32v003fun` - 1 edges
10. `BDR612H Motor Driver` - 1 edges

## Surprising Connections (you probably didn't know these)
- `CH32V003` --semantically_similar_to--> `CH32V003J4M6 Microcontroller`  [INFERRED] [semantically similar]
  handoff.md → initial_bom.md
- `CH32V003` --conceptually_related_to--> `Programmable Gain Amplifier`  [INFERRED]
  handoff.md → CH32V003.pdf
- `PY32F002A` --conceptually_related_to--> `Analog-to-Digital Converter`  [INFERRED]
  handoff.md → CH32V003RM-English.pdf
- `BDR612H` --semantically_similar_to--> `BDR612H Motor Driver`  [INFERRED] [semantically similar]
  handoff.md → initial_bom.md
- `RC Low-Pass Filter` --conceptually_related_to--> `10µF Filter Capacitor`  [EXTRACTED]
  handoff.md → initial_bom.md

## Hyperedges (group relationships)
- **Smart Servo BOM Core Components** — handoff_ch32v003, handoff_bdr612h, handoff_smaj5_0a, handoff_mg996r [EXTRACTED 1.00]
- **Smart Servo Firmware Modules** — handoff_pid_loop, handoff_dma_adc, handoff_deadtime_insertion [INFERRED 0.85]

## Communities (12 total, 8 thin omitted)

### Community 0 - "Smart Servo Overview"
Cohesion: 0.4
Nodes (5): Programmable Gain Amplifier, CH32V003, ch32v003fun, DMA ADC, CH32V003J4M6 Microcontroller

### Community 1 - "CH32V003 Integration"
Cohesion: 0.67
Nodes (3): TowerPro MG996R, Smart Servo Conversion Project, Star Grounding Topology

### Community 2 - "BDR612H Motor Driver"
Cohesion: 0.67
Nodes (3): RC Low-Pass Filter, 10Ω Filter Resistor, 10µF Filter Capacitor

### Community 3 - "Signal Filtering"
Cohesion: 0.67
Nodes (3): BDR612H, Deadtime Insertion, BDR612H Motor Driver

## Knowledge Gaps
- **18 isolated node(s):** `Star Grounding Topology`, `ch32v003fun`, `BDR612H Motor Driver`, `CH32V003J4M6 Microcontroller`, `SMAJ5.0A TVS Diode` (+13 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **8 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `CH32V003` connect `Smart Servo Overview` to `CH32V003 Integration`, `BDR612H Motor Driver`, `Signal Filtering`, `Programming / Flashing`, `Analog Current Sensing`, `TVS Protection`?**
  _High betweenness centrality (0.576) - this node is a cross-community bridge._
- **Why does `Smart Servo Conversion Project` connect `CH32V003 Integration` to `Smart Servo Overview`?**
  _High betweenness centrality (0.127) - this node is a cross-community bridge._
- **Why does `RC Low-Pass Filter` connect `BDR612H Motor Driver` to `Smart Servo Overview`?**
  _High betweenness centrality (0.127) - this node is a cross-community bridge._
- **Are the 8 inferred relationships involving `CH32V003` (e.g. with `RC Low-Pass Filter` and `CH32V003J4M6 Microcontroller`) actually correct?**
  _`CH32V003` has 8 INFERRED edges - model-reasoned connections that need verification._
- **Are the 3 inferred relationships involving `BDR612H` (e.g. with `CH32V003` and `BDR612H Motor Driver`) actually correct?**
  _`BDR612H` has 3 INFERRED edges - model-reasoned connections that need verification._
- **Are the 2 inferred relationships involving `PY32F002A` (e.g. with `CH32V003` and `Analog-to-Digital Converter`) actually correct?**
  _`PY32F002A` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Star Grounding Topology`, `ch32v003fun`, `BDR612H Motor Driver` to the rest of the system?**
  _18 weakly-connected nodes found - possible documentation gaps or missing edges._