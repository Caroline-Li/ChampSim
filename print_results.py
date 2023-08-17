#!/usr/bin/python3
import sys, re
import matplotlib.pyplot as plt

if len(sys.argv) < 3:
    print('Need to specify a baseline!')
    print('Example usage: ./print_results.py <baseline-experiment> <comparison-experiment>')
    sys.exit(0)

baseline = sys.argv[1]
comparison = sys.argv[2]

trace_list = open('sim_list/traces.txt', 'r')
base_trace_files = []
comp_trace_files = []
traces = []
for trace in trace_list:
    trace = trace[:-1]
    traces.append(trace)
    base_trace = 'output/'+baseline+'/'+trace+'.txt'
    base_trace_files.append(base_trace)
    comp_trace = 'output/'+comparison+'/'+trace+'.txt'
    comp_trace_files.append(comp_trace)

def search(regex, pos, trace_files):
    result = []
    for trace_file in trace_files:
        found = False
        for line in open(trace_file, 'r'):
            if not found and re.match(regex, line):
                result.append(float(line.split()[pos]))
                found = True
    return result

baseline_misses = search('LLC LOAD', 7, base_trace_files)
comparison_misses = search('LLC LOAD', 7, comp_trace_files)
coverage = [(baseline_misses[i]-comparison_misses[i])/baseline_misses[i] for i in range(len(traces))]
baseline_ipc = search('CPU 0 cumulative IPC', 4, base_trace_files)
comparison_ipc = search('CPU 0 cumulative IPC', 4, comp_trace_files)
speedup = [comparison_ipc[i]/baseline_ipc[i] for i in range(len(traces))]
accuracy = search('L2C ACCURACY:', 2, comp_trace_files)
DRAM_base_hit = search(' RQ ROW_BUFFER_HIT: ', 2, base_trace_files)
DRAM_comp_hit = search(' RQ ROW_BUFFER_HIT:', 2, comp_trace_files)
DRAM_base_miss = search(' RQ ROW_BUFFER_HIT:', 4, base_trace_files)
DRAM_comp_miss = search(' RQ ROW_BUFFER_HIT:', 4, comp_trace_files)
DRAM_base_total = [DRAM_base_hit[i] + DRAM_base_miss[i] for i in range(len(traces))]
DRAM_comp_total = [DRAM_comp_hit[i] + DRAM_comp_miss[i] for i in range(len(traces))]
DRAM_diff = [DRAM_comp_total[i] - DRAM_base_total[i] for i  in range(len(traces))]
hitRate_base = search('OPTgen hit rate: ', 3, base_trace_files)
hitRate_comp = search('OPTgen hit rate: ', 3, comp_trace_files)
matchCombined = re.search(r'glider_combined\w*', comparison)
if (matchCombined):
    pcEntries = search('SHCT SIZE:', 2, comp_trace_files)
    IsvmEntries = search('SIZE:', 1, comp_trace_files)
    print(pcEntries)
    print(IsvmEntries)
    SHCTsizeTotal = 0
    isvmTotal = 0
    averageShctSize = 0.0
    averageIsvmSize = 0.0
    for i in range(len(traces)):
        SHCTsizeTotal += pcEntries[i]
        isvmTotal += IsvmEntries[i]
    averageShctSize = SHCTsizeTotal / (len(traces))
    averageIsvmSize = isvmTotal / (len(traces))
    print(averageShctSize)
    print(averageIsvmSize)
    percent_SHCT = search('Ratio of Hawkeye SHCT Predictions: ', 5, comp_trace_files)
    percent_history = search('Ratio of history table Predictions: ', 5, comp_trace_files)
    average_SHCT = 0.0
    average_history = 0.0
    SHCT_total = 0
    history_total = 0
    for i in range(len(traces)):
        SHCT_total += percent_SHCT[i]
        history_total += percent_history[i]
        
    average_SHCT = SHCT_total / (len(traces))
    average_history = history_total / (len(traces))
    print(percent_SHCT)
    print(percent_history)
    print(average_SHCT)
    print(average_history)
    
    
matchAccuracy = re.search(r'\w*accuracy\w*', comparison)
if (matchAccuracy):
    print("ACCURACY")
    accuracySHCT = search('AVERAGE SHCT ACCURACY LOW BIAS:', 5, comp_trace_files)
    accuracyPerceptron = search('AVERAGE PERCEPTRON ACCURACY LOW BIAS:', 5, comp_trace_files)
    print(accuracySHCT)
    print(accuracyPerceptron)

match = re.search(r'hawkeye_final_dyn\w*', comparison) or re.search(r'harmony\w*', comparison)
if match:
    dd_accuracy = search('D-D intervals:', 3, comp_trace_files)
    dp_accuracy = search('D-P intervals:', 3, comp_trace_files)
    pd_accuracy = search('P-D intervals:', 3, comp_trace_files)
    pp_accuracy = search('P-P intervals:', 3, comp_trace_files)

match2 = re.search(r'harmony_suppress\w*', comparison)
if match2:
    pred_true = search('Prefetch Predictions: ', 3, comp_trace_files)
    pred_false = search('Prefetch Predictions: ', 5, comp_trace_files)
matchFlex = re.search(r'\w*glider\w*', comparison)
matchHarmony = re.search(r'\w*final\w*', comparison)
if (matchFlex or matchHarmony):
    print("OPTgen hit rates")
    compOpt = search('OPTgen hit rate: ', 3, comp_trace_files)
    print(compOpt)



    


print('Prefetcher Performance')
print('Coverage,     Speedup,     D-D accuracy,     D-P accuracy,     P-D accuracy,     P-P accuracy %,    Benchmark')
print('-----------------------------')
cov_sum = 0.0
acc_sum = 0.0
ipc_sum = 0.0
dd_sum = 0.0
dp_sum = 0.0
pd_sum = 0.0
pp_sum = 0.0
DRAM_diff_total = 0.0
for i in range(len(traces)):
    if not match:
        print('%5.3f         %5.3f         %s' % (coverage[i], speedup[i], traces[i]))
    elif match2:
        print('%5.3f         %5.3f         %5.3f         %5.3f           %5.3f             %5.3f           %5.3f             %s' % (coverage[i], speedup[i], dd_accuracy[i], dp_accuracy[i], pd_accuracy[i], pp_accuracy[i], DRAM_diff[i], traces[i]))
    elif match:
        print('%5.3f         %5.3f         %5.3f         %5.3f           %5.3f             %5.3f           %s' % (coverage[i], speedup[i], dd_accuracy[i], dp_accuracy[i], pd_accuracy[i], pp_accuracy[i], traces[i]))
    cov_sum += coverage[i]
    ipc_sum += speedup[i]
    if match:
        dd_sum += dd_accuracy[i]
        dp_sum += dp_accuracy[i]
        pd_sum += pd_accuracy[i]
        pp_sum += pp_accuracy[i]
        DRAM_diff_total += DRAM_diff[i]
        

# print('%5.3f         %5.3f         %5.4f         %5.3f           %5.3f             %5.3f           %5.3f            %5.3f        average' % (cov_sum/(len(traces)), acc_sum/(len(traces)), ipc_sum/(len(traces)), dd_sum/(len(traces)), dp_sum/(len(traces)), pd_sum/(len(traces)), pp_sum/(len(traces)), DRAM_diff_total/(len(traces))))

