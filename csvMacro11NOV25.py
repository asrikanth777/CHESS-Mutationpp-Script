#tips
#paraview gets confused if you run a script from home, so
#open the folder you want to work in with your exec,input,mesh,output files
#then open folder in terminal and type paraview
#from there you can select your script and have it work from there
#the print statements are to double check you are in the path you want to work from

from pathlib import Path
from paraview.simple import *
import re

paraview.simple._DisableFirstRenderCameraReset()

base = Path.cwd()
print(base)
print(str(base))
name = input("name for folder with pvts and vts")
folder = base / name
print(folder)
name2 = input("name for folder to export csvs")
output = base / name2


pvts_files = sorted(folder.glob("*.pvts"))
vts_files  = sorted(folder.glob("*.vts"))

# Build a set of base stems from pvts
pvts_stems = [f.stem[-2:] for f in pvts_files]
pvts_stem1 = [f.stem for f in pvts_files]

print("pvts_stems:", pvts_stems)
print("pvts_stem1:", pvts_stem1)

# Filter vts_files: keep only those whose stem does NOT start with any pvts stem
clean_vts = []
for v in vts_files:
    if not any(v.stem.endswith(stem) for stem in pvts_stems):
        clean_vts.append(v)

print("PVTS:", [f.name for f in pvts_files])
print("Filtered VTS:", [f.name for f in clean_vts])

vts_stem1 = [f.stem for f in clean_vts]

# Make readers (unchanged)
pvts_readers = [XMLPartitionedStructuredGridReader(FileName=str(f)) for f in pvts_files]
vts_readers  = [XMLStructuredGridReader(FileName=str(f)) for f in clean_vts]
renderView1 = GetActiveViewOrCreate('RenderView')

print(pvts_readers)
print(vts_readers)
spreadSheetView1 = CreateView('SpreadSheetView')
plength = len(pvts_files)
vlength = len(clean_vts)
i = 0
j = 0

while i < plength:
	cellDatatoPointDataPVTS = CellDatatoPointData(Input=pvts_readers[i])
	cellDatatoPointDataPVTS.CellDataArraytoprocess = ['H', 'M', 'T', 'p', 'rho', 'v']
	cellDatatoPointDataSheet = Show(pvts_readers[i], spreadSheetView1)
	ExportView(str(output) +'/' + pvts_stem1[i] + '.csv', view=spreadSheetView1)
	i+= 1



while j < vlength:
	cellDatatoPointDataPVTS = CellDatatoPointData(Input=vts_readers[j])
	cellDatatoPointDataPVTS.CellDataArraytoprocess = ['H', 'M', 'T', 'p', 'rho', 'v']
	cellDatatoPointDataSheet = Show(vts_readers[j], spreadSheetView1)
	ExportView(str(output) +'/' + vts_stem1[j] + '.csv', view=spreadSheetView1)
	j+= 1


