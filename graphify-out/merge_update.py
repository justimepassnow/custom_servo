import json
from pathlib import Path
from networkx.readwrite import json_graph
import networkx as nx
from graphify.build import build_from_json
import shutil

# Load existing graph
existing_data = json.loads(Path('graphify-out/graph.json').read_text(encoding='utf-8'))
G_existing = json_graph.node_link_graph(existing_data, edges='links')

# Load new extraction
new_extraction = json.loads(Path('graphify-out/.graphify_semantic_new.json').read_text(encoding='utf-8'))
G_new = build_from_json(new_extraction)

# Prune nodes from deleted/changed files to avoid ghosts
incremental = json.loads(Path('graphify-out/.graphify_incremental.json').read_text(encoding='utf-8'))
changed_files = [f for files in incremental.get('new_files', {}).values() for f in files]
if changed_files:
    to_remove = [n for n, d in G_existing.nodes(data=True) if d.get('source_file') in changed_files]
    G_existing.remove_nodes_from(to_remove)
    print(f'Pruned {len(to_remove)} ghost node(s) from changed files.')

# Merge: new nodes/edges into existing graph
G_existing.update(G_new)
print(f'Merged: {G_existing.number_of_nodes()} nodes, {G_existing.number_of_edges()} edges')

# Write merged result back to .graphify_extract.json so Step 4 sees the full graph
merged_out = {
    'nodes': [{'id': n, **d} for n, d in G_existing.nodes(data=True)],
    'edges': [{'source': u, 'target': v, **d} for u, v, d in G_existing.edges(data=True)],
    'hyperedges': new_extraction.get('hyperedges', []),
    'input_tokens': new_extraction.get('input_tokens', 0),
    'output_tokens': new_extraction.get('output_tokens', 0),
}
Path('graphify-out/.graphify_extract.json').write_text(json.dumps(merged_out), encoding='utf-8')
print(f'[graphify update] Merged extraction written ({len(merged_out["nodes"])} nodes, {len(merged_out["edges"])} edges)')

from graphify.detect import save_manifest
save_manifest(incremental['files'])
print('[graphify update] Manifest saved.')