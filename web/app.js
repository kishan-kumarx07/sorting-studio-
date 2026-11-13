const algorithms = {
  merge_sort: {
    label: 'Merge Sort',
    stable: true,
    complexity: ['O(n log n)', 'O(n log n)', 'O(n log n)', 'O(n)', 'Yes'],
    badge: 'Stable and predictable',
    pseudocode: `mergeSort(array):
  if array has 0 or 1 item return
  split array into left and right halves
  mergeSort(left)
  mergeSort(right)
  merge two sorted halves back together`,
    code: `void mergeSort(vector<int>& values, int left, int right) {
  if (left >= right) return;
  int mid = left + (right - left) / 2;
  mergeSort(values, left, mid);
  mergeSort(values, mid + 1, right);
  merge(values, left, mid, right);
}`,
    explanation: 'Merge Sort splits the array into smaller pieces until each piece is trivial to sort, then merges the pieces back together in sorted order. It is stable, which means equal values keep their original relative order.',
  },
  quick_sort: {
    label: 'Quick Sort',
    stable: false,
    complexity: ['O(n log n)', 'O(n log n)', 'O(n^2)', 'O(log n)', 'No'],
    badge: 'Fast average case',
    pseudocode: `quickSort(array, low, high):
  if low < high:
    pivotIndex = partition(array, low, high)
    quickSort(left side)
    quickSort(right side)`,
    code: `int partition(vector<int>& values, int low, int high) {
  int pivot = values[high];
  int i = low - 1;
  for (int j = low; j < high; ++j) {
    if (values[j] <= pivot) swap(values[++i], values[j]);
  }
  swap(values[i + 1], values[high]);
  return i + 1;
}`,
    explanation: 'Quick Sort picks a pivot value and partitions the array so smaller values go to one side and larger values go to the other. It is usually very fast, but a poor pivot choice can make it much slower.',
  },
  heap_sort: {
    label: 'Heap Sort',
    stable: false,
    complexity: ['O(n log n)', 'O(n log n)', 'O(n log n)', 'O(1)', 'No'],
    badge: 'In-place and memory efficient',
    pseudocode: `heapSort(array):
  build a max heap
  swap the root with the last item
  reduce heap size
  heapify the root again
  repeat until sorted`,
    code: `void heapSort(vector<int>& values) {
  buildMaxHeap(values);
  for (int end = values.size() - 1; end > 0; --end) {
    swap(values[0], values[end]);
    heapify(values, end, 0);
  }
}`,
    explanation: 'Heap Sort first organizes the data into a max heap. Then it repeatedly takes the largest value from the top of the heap and places it at the end of the array.',
  },
  counting_sort: {
    label: 'Counting Sort',
    stable: true,
    complexity: ['O(n + k)', 'O(n + k)', 'O(n + k)', 'O(n + k)', 'Yes'],
    badge: 'Great for small ranges',
    pseudocode: `countingSort(array):
  count how many times each value appears
  convert counts to prefix sums
  place items into the output array from right to left
  copy the output back`,
    code: `void countingSort(vector<int>& values) {
  int minValue = *min_element(values.begin(), values.end());
  int maxValue = *max_element(values.begin(), values.end());
  vector<int> counts(maxValue - minValue + 1, 0);
  for (int value : values) counts[value - minValue]++;
  for (size_t i = 1; i < counts.size(); ++i) counts[i] += counts[i - 1];
}`,
    explanation: 'Counting Sort does not compare values directly. It counts how many times each number appears, then uses those counts to place every value into its final position.',
  },
  radix_sort: {
    label: 'Radix Sort',
    stable: true,
    complexity: ['O(nk)', 'O(nk)', 'O(nk)', 'O(n + k)', 'Yes'],
    badge: 'Digit-by-digit sorting',
    pseudocode: `radixSort(array):
  find the largest number
  sort by ones digit
  sort by tens digit
  sort by hundreds digit
  continue until all digits are processed`,
    code: `void radixSort(vector<int>& values) {
  for (int exp = 1; maxValue / exp > 0; exp *= 10) {
    countingSortByDigit(values, exp);
  }
}`,
    explanation: 'Radix Sort sorts numbers one digit at a time. It keeps each digit pass stable, which is why the ordering from earlier passes is preserved.',
  },
  bucket_sort: {
    label: 'Bucket Sort',
    stable: true,
    complexity: ['O(n + k)', 'O(n + k)', 'O(n^2)', 'O(n + k)', 'Yes'],
    badge: 'Smooth for spread-out data',
    pseudocode: `bucketSort(array):
  create empty buckets
  distribute values into buckets
  sort each bucket separately
  combine the buckets back together`,
    code: `void bucketSort(vector<int>& values) {
  createBuckets(values);
  sortEachBucket();
  mergeBucketsBack(values);
}`,
    explanation: 'Bucket Sort divides the values into ranges, sorts each range, and then joins the results. It works best when the data is spread fairly evenly across the range.',
  },
};

const algorithmOrder = Object.keys(algorithms);
const apiBase = window.location.port === '8080' ? '' : 'http://localhost:8080';

const state = {
  theme: localStorage.getItem('smart_sort_theme') || 'dark',
  learn: {
    algorithm: 'merge_sort',
    dataset: [],
    trace: null,
    step: 0,
    playing: false,
    timer: null,
    speed: 2,
  },
  compare: {
    algorithmA: 'quick_sort',
    algorithmB: 'merge_sort',
    dataset: [],
    leftTrace: null,
    rightTrace: null,
    leftStep: 0,
    rightStep: 0,
    playing: false,
    timer: null,
    speed: 2,
  },
};

const elements = {};

function $(id) {
  return document.getElementById(id);
}

function setTheme(theme) {
  state.theme = theme;
  document.body.classList.toggle('theme-light', theme === 'light');
  localStorage.setItem('smart_sort_theme', theme);
}

function speedToDelay(value) {
  if (value === 1) return 900;
  if (value === 2) return 360;
  return 110;
}

function speedToLabel(value) {
  return value === 1 ? 'Slow' : value === 2 ? 'Medium' : 'Fast';
}

function algorithmLabel(name) {
  return algorithms[name]?.label || name;
}

function complexityText(name) {
  return algorithms[name]?.complexity || ['-', '-', '-', '-', '-'];
}

function randomDataset(size) {
  const values = [];
  for (let i = 0; i < size; i += 1) {
    const base = Math.floor(Math.random() * 90) + 10;
    const signed = Math.random() < 0.15 ? -base : base;
    values.push(signed);
  }
  return values;
}

function parseManualDataset(text) {
  if (!text.trim()) return [];
  return text
    .split(/[^-\d]+/)
    .map((value) => Number(value))
    .filter((value) => Number.isFinite(value));
}

function parseCsvToArray(csvText) {
  const numbers = [];
  csvText
    .replace(/\r/g, '\n')
    .split(/[\n,;]/)
    .map((part) => part.trim())
    .forEach((part) => {
      if (!part) return;
      const number = Number(part);
      if (Number.isFinite(number)) numbers.push(number);
    });
  return numbers;
}

function formatDuration(ms) {
  return `${ms.toFixed(2)} ms`;
}

function updateBackendStatus(text, ok = true) {
  elements.backendStatus.textContent = text;
  elements.backendStatus.classList.toggle('status-pill', ok);
}

function showScreen(screenId) {
  ['homeScreen', 'learnScreen', 'compareScreen'].forEach((id) => {
    $(id).classList.toggle('screen-active', id === screenId);
  });
}

function populateAlgorithmSelect(select) {
  select.innerHTML = algorithmOrder
    .map((key) => `<option value="${key}">${algorithms[key].label}</option>`)
    .join('');
}

function applyAlgorithmMetaToLearn() {
  const algo = algorithms[state.learn.algorithm];
  elements.learnPseudocode.textContent = algo.pseudocode;
  elements.learnCode.textContent = algo.code;
  elements.learnExplanation.textContent = algo.explanation;
  elements.learnComplexityBadge.textContent = algo.badge;
  elements.learnStableBadge.textContent = algo.stable ? 'Stable' : 'Unstable';
  elements.learnStableBadge.dataset.state = algo.stable ? 'stable' : 'unstable';

  const [best, average, worst, space, stable] = algo.complexity;
  elements.learnComplexityRow.innerHTML = `
    <td>${best}</td>
    <td>${average}</td>
    <td>${worst}</td>
    <td>${space}</td>
    <td>${stable}</td>
  `;
}

function renderBars(container, array, step = null, mode = 'single') {
  container.innerHTML = '';
  const values = array.length ? array : [0];
  const maxAbs = Math.max(1, ...values.map((value) => Math.abs(value)));
  const minAbs = Math.min(...values.map((value) => Math.abs(value)));
  const template = $("barTemplate");
  const activeIndices = new Set(step?.indices || []);
  const action = step?.action || 'idle';

  values.forEach((value, index) => {
    const fragment = template.content.cloneNode(true);
    const slot = fragment.querySelector('.bar-slot');
    const bar = fragment.querySelector('.bar');
    const label = fragment.querySelector('.bar-value');
    const height = Math.max(4, (Math.abs(value) / maxAbs) * 100);
    bar.style.height = `${height}%`;
    if (Math.abs(value) === minAbs) {
      bar.classList.add('value-alt');
    }
    if (activeIndices.has(index)) {
      bar.classList.add(`active-${action}`);
    }
    if (mode === 'comparison') {
      if (index % 2 === 0) {
        bar.classList.add('value-alt');
      }
    }
    label.textContent = value;
    if (values.length > 18) {
      label.style.opacity = '0.45';
    }
    slot.title = `Index ${index}: ${value}`;
    container.appendChild(fragment);
  });
}

function formatStep(step) {
  if (!step) return 'Idle';
  const map = {
    start: 'Starting animation',
    compare: 'Comparing values',
    swap: 'Swapping positions',
    write: 'Writing values',
    count: 'Counting values',
    scatter: 'Distributing into buckets',
    keep: 'Value already in place',
    done: 'Finished',
  };
  return map[step.action] || step.action;
}

async function fetchJson(url, options) {
  const response = await fetch(url, options);
  const payload = await response.json();
  if (!response.ok || payload.error) {
    throw new Error(payload.error || `Request failed: ${response.status}`);
  }
  return payload;
}

async function fetchTrace(algorithm, dataset) {
  return fetchJson(`${apiBase}/api/sort`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ algorithm, dataset }),
  });
}

async function fetchComparison(algorithm1, algorithm2, dataset) {
  return fetchJson(`${apiBase}/api/compare`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ algorithm1, algorithm2, dataset }),
  });
}

function stopTimer(which) {
  const target = which === 'learn' ? state.learn : state.compare;
  if (target.timer) {
    clearTimeout(target.timer);
    target.timer = null;
  }
}

function scheduleLearnFrame() {
  stopTimer('learn');
  if (!state.learn.playing || !state.learn.trace) return;
  state.learn.timer = setTimeout(() => {
    stepLearnForward(true);
  }, speedToDelay(state.learn.speed));
}

function scheduleCompareFrame() {
  stopTimer('compare');
  if (!state.compare.playing || !state.compare.leftTrace || !state.compare.rightTrace) return;
  state.compare.timer = setTimeout(() => {
    stepCompareForward(true);
  }, speedToDelay(state.compare.speed));
}

function updateLearnView() {
  const trace = state.learn.trace;
  if (!trace) {
    renderBars(elements.learnChart, state.learn.dataset, null, 'single');
    elements.learnStepLabel.textContent = 'Idle';
    return;
  }
  const step = trace.steps[Math.min(state.learn.step, trace.steps.length - 1)] || trace.steps[trace.steps.length - 1];
  renderBars(elements.learnChart, step.array, step, 'single');
  elements.learnStepLabel.textContent = `${formatStep(step)} | ${state.learn.step + 1}/${trace.steps.length}`;
}

function updateCompareView() {
  const leftTrace = state.compare.leftTrace;
  const rightTrace = state.compare.rightTrace;
  if (!leftTrace || !rightTrace) {
    renderBars(elements.algoAChart, state.compare.dataset, null, 'comparison');
    renderBars(elements.algoBChart, state.compare.dataset, null, 'comparison');
    elements.algoAStepLabel.textContent = 'Idle';
    elements.algoBStepLabel.textContent = 'Idle';
    return;
  }
  const leftStep = leftTrace.steps[Math.min(state.compare.leftStep, leftTrace.steps.length - 1)] || leftTrace.steps[leftTrace.steps.length - 1];
  const rightStep = rightTrace.steps[Math.min(state.compare.rightStep, rightTrace.steps.length - 1)] || rightTrace.steps[rightTrace.steps.length - 1];
  renderBars(elements.algoAChart, leftStep.array, leftStep, 'comparison');
  renderBars(elements.algoBChart, rightStep.array, rightStep, 'comparison');
  elements.algoAStepLabel.textContent = `${formatStep(leftStep)} | ${state.compare.leftStep + 1}/${leftTrace.steps.length}`;
  elements.algoBStepLabel.textContent = `${formatStep(rightStep)} | ${state.compare.rightStep + 1}/${rightTrace.steps.length}`;
}

function stepLearnForward(managed = false) {
  if (!state.learn.trace) return;
  state.learn.step = Math.min(state.learn.step + 1, state.learn.trace.steps.length - 1);
  updateLearnView();
  if (state.learn.step >= state.learn.trace.steps.length - 1) {
    state.learn.playing = false;
    elements.learnPause.textContent = 'Pause';
    stopTimer('learn');
    return;
  }
  if (managed && state.learn.playing) {
    scheduleLearnFrame();
  }
}

function stepLearnBack() {
  if (!state.learn.trace) return;
  state.learn.step = Math.max(0, state.learn.step - 1);
  updateLearnView();
}

function restartLearn() {
  if (!state.learn.trace) return;
  state.learn.step = 0;
  updateLearnView();
  if (state.learn.playing) scheduleLearnFrame();
}

function toggleLearnPlayback() {
  if (!state.learn.trace) return;
  state.learn.playing = !state.learn.playing;
  elements.learnPause.textContent = state.learn.playing ? 'Pause' : 'Resume';
  if (state.learn.playing) scheduleLearnFrame();
  else stopTimer('learn');
}

function toggleComparePlayback() {
  if (!state.compare.leftTrace || !state.compare.rightTrace) return;
  state.compare.playing = !state.compare.playing;
  elements.comparePause.textContent = state.compare.playing ? 'Pause' : 'Resume';
  if (state.compare.playing) {
    scheduleCompareFrame();
  } else {
    stopTimer('compare');
  }
}

function stepCompareForward(managed = false) {
  if (!state.compare.leftTrace || !state.compare.rightTrace) return;
  state.compare.leftStep = Math.min(state.compare.leftStep + 1, state.compare.leftTrace.steps.length - 1);
  state.compare.rightStep = Math.min(state.compare.rightStep + 1, state.compare.rightTrace.steps.length - 1);
  updateCompareView();
  const finishedLeft = state.compare.leftStep >= state.compare.leftTrace.steps.length - 1;
  const finishedRight = state.compare.rightStep >= state.compare.rightTrace.steps.length - 1;
  if (finishedLeft && finishedRight) {
    state.compare.playing = false;
    stopTimer('compare');
    return;
  }
  if (managed && state.compare.playing) {
    scheduleCompareFrame();
  }
}

function restartCompare() {
  if (!state.compare.leftTrace || !state.compare.rightTrace) return;
  state.compare.leftStep = 0;
  state.compare.rightStep = 0;
  updateCompareView();
  if (state.compare.playing) scheduleCompareFrame();
}

function renderMetrics(targetElement, result, metadata) {
  targetElement.innerHTML = `
    <span class="metric-chip"><strong>${formatDuration(result.durationMs)}</strong></span>
    <span class="metric-chip"><strong>${result.metrics.comparisons}</strong> comparisons</span>
    <span class="metric-chip"><strong>${result.metrics.swaps}</strong> swaps</span>
    <span class="metric-chip">${metadata.complexity[0]}</span>
  `;
}

function updateComparisonSummary(left, right) {
  const winnerLabel = algorithmLabel(left.algorithm === right.algorithm ? left.algorithm : (left.durationMs <= right.durationMs ? left.algorithm : right.algorithm));
  const message = left.durationMs <= right.durationMs
    ? `${algorithmLabel(left.algorithm)} performed better for this dataset.`
    : `${algorithmLabel(right.algorithm)} performed better for this dataset.`;
  elements.compareSummary.textContent = message;
  elements.compareSummary.classList.remove('good', 'bad');
  elements.compareSummary.classList.add('good');
  elements.algoAName.textContent = algorithmLabel(left.algorithm);
  elements.algoBName.textContent = algorithmLabel(right.algorithm);
  elements.algoALabel.textContent = left.stable ? 'Stable algorithm' : 'Unstable algorithm';
  elements.algoBLabel.textContent = right.stable ? 'Stable algorithm' : 'Unstable algorithm';
  elements.algoAMetrics.innerHTML = `
    <span class="metric-chip"><strong>${formatDuration(left.durationMs)}</strong></span>
    <span class="metric-chip"><strong>${left.metrics.comparisons}</strong> comparisons</span>
    <span class="metric-chip"><strong>${left.metrics.swaps}</strong> swaps</span>
  `;
  elements.algoBMetrics.innerHTML = `
    <span class="metric-chip"><strong>${formatDuration(right.durationMs)}</strong></span>
    <span class="metric-chip"><strong>${right.metrics.comparisons}</strong> comparisons</span>
    <span class="metric-chip"><strong>${right.metrics.swaps}</strong> swaps</span>
  `;
  elements.compareSummary.title = `${winnerLabel} is faster on this dataset`;
}

async function runLearning() {
  const algorithm = elements.learnAlgorithm.value;
  state.learn.algorithm = algorithm;
  applyAlgorithmMetaToLearn();
  const dataset = state.learn.dataset.length ? state.learn.dataset : randomDataset(Number(elements.learnSize.value));
  state.learn.dataset = dataset;
  try {
    const trace = await fetchTrace(algorithm, dataset);
    state.learn.trace = trace;
    state.learn.step = 0;
    state.learn.playing = true;
    elements.learnPause.textContent = 'Pause';
    renderBars(elements.learnChart, trace.steps[0]?.array || dataset, trace.steps[0] || null, 'single');
    elements.learnDatasetLabel.textContent = `Dataset: ${dataset.join(', ')}`;
    updateLearnView();
    scheduleLearnFrame();
  } catch (error) {
    updateBackendStatus(error.message, false);
    elements.compareSummary.textContent = `Comparison failed: ${error.message}`;
    elements.compareSummary.classList.remove('good');
    elements.compareSummary.classList.add('bad');
  }
}

async function runComparison() {
  const algorithmA = elements.compareAlgorithmA.value;
  const algorithmB = elements.compareAlgorithmB.value;
  let dataset = parseManualDataset(elements.manualDataset.value);
  if (!dataset.length) {
    dataset = state.compare.dataset.length ? state.compare.dataset : randomDataset(Number(elements.compareSize.value));
  }
  state.compare.dataset = dataset;
  try {
    const result = await fetchComparison(algorithmA, algorithmB, dataset);
    state.compare.leftTrace = result.left;
    state.compare.rightTrace = result.right;
    state.compare.leftStep = 0;
    state.compare.rightStep = 0;
    state.compare.playing = true;
    renderBars(elements.algoAChart, result.left.steps[0]?.array || dataset, result.left.steps[0] || null, 'comparison');
    renderBars(elements.algoBChart, result.right.steps[0]?.array || dataset, result.right.steps[0] || null, 'comparison');
    updateComparisonSummary(result.left, result.right);
    updateCompareView();
    scheduleCompareFrame();
  } catch (error) {
    updateBackendStatus(error.message, false);
  }
}

function syncLearnDatasetLabel() {
  elements.learnSizeValue.textContent = elements.learnSize.value;
  elements.speedLabel.textContent = speedToLabel(Number(elements.speedControl.value));
  state.learn.speed = Number(elements.speedControl.value);
}

function syncCompareDatasetLabel() {
  elements.compareSizeValue.textContent = elements.compareSize.value;
  state.compare.speed = Number(elements.speedControl.value);
}

function loadSampleDataset() {
  fetch(`${apiBase}/data/sample_dataset.csv`)
    .then((response) => response.text())
    .then((text) => {
      const numbers = parseCsvToArray(text);
      elements.manualDataset.value = numbers.join(', ');
      state.compare.dataset = numbers;
      updateBackendStatus(`Loaded sample dataset with ${numbers.length} values.`);
    })
    .catch((error) => updateBackendStatus(error.message, false));
}

function wireEvents() {
  elements.goLearn.addEventListener('click', () => showScreen('learnScreen'));
  elements.goCompare.addEventListener('click', () => showScreen('compareScreen'));
  elements.backFromLearn.addEventListener('click', () => showScreen('homeScreen'));
  elements.backFromCompare.addEventListener('click', () => showScreen('homeScreen'));
  elements.themeToggle.addEventListener('click', () => setTheme(state.theme === 'dark' ? 'light' : 'dark'));

  elements.learnAlgorithm.addEventListener('change', () => {
    state.learn.algorithm = elements.learnAlgorithm.value;
    applyAlgorithmMetaToLearn();
  });

  elements.learnSize.addEventListener('input', () => {
    elements.learnSizeValue.textContent = elements.learnSize.value;
  });

  elements.speedControl.addEventListener('input', () => {
    state.learn.speed = Number(elements.speedControl.value);
    state.compare.speed = Number(elements.speedControl.value);
    elements.speedLabel.textContent = speedToLabel(Number(elements.speedControl.value));
  });

  elements.learnGenerate.addEventListener('click', () => {
    state.learn.dataset = randomDataset(Number(elements.learnSize.value));
    elements.learnDatasetLabel.textContent = `Dataset: ${state.learn.dataset.join(', ')}`;
    state.learn.trace = null;
    state.learn.step = 0;
    renderBars(elements.learnChart, state.learn.dataset, null, 'single');
    elements.learnStepLabel.textContent = 'Dataset ready';
  });

  elements.learnRun.addEventListener('click', runLearning);
  elements.learnPause.addEventListener('click', toggleLearnPlayback);
  elements.learnRestart.addEventListener('click', restartLearn);
  elements.learnStepBack.addEventListener('click', stepLearnBack);
  elements.learnStepForward.addEventListener('click', () => stepLearnForward(false));

  elements.compareAlgorithmA.addEventListener('change', () => {
    state.compare.algorithmA = elements.compareAlgorithmA.value;
  });
  elements.compareAlgorithmB.addEventListener('change', () => {
    state.compare.algorithmB = elements.compareAlgorithmB.value;
  });
  elements.compareSize.addEventListener('input', () => {
    elements.compareSizeValue.textContent = elements.compareSize.value;
  });

  elements.generateComparison.addEventListener('click', () => {
    state.compare.dataset = randomDataset(Number(elements.compareSize.value));
    elements.manualDataset.value = state.compare.dataset.join(', ');
    updateBackendStatus(`Generated dataset with ${state.compare.dataset.length} values.`);
  });

  elements.loadSampleDataset.addEventListener('click', loadSampleDataset);
  elements.csvInput.addEventListener('change', async (event) => {
    const file = event.target.files?.[0];
    if (!file) return;
    const text = await file.text();
    const numbers = parseCsvToArray(text);
    elements.manualDataset.value = numbers.join(', ');
    state.compare.dataset = numbers;
    updateBackendStatus(`CSV loaded with ${numbers.length} values.`);
  });

  elements.runComparison.addEventListener('click', runComparison);
  elements.comparePause.addEventListener('click', toggleComparePlayback);
  elements.compareSummary.addEventListener('click', restartCompare);
}

function initSelects() {
  populateAlgorithmSelect(elements.learnAlgorithm);
  populateAlgorithmSelect(elements.compareAlgorithmA);
  populateAlgorithmSelect(elements.compareAlgorithmB);
  elements.learnAlgorithm.value = state.learn.algorithm;
  elements.compareAlgorithmA.value = state.compare.algorithmA;
  elements.compareAlgorithmB.value = state.compare.algorithmB;
}

function initDefaults() {
  state.learn.dataset = randomDataset(Number(elements.learnSize.value));
  state.compare.dataset = randomDataset(Number(elements.compareSize.value));
  elements.learnDatasetLabel.textContent = `Dataset: ${state.learn.dataset.join(', ')}`;
  elements.manualDataset.value = state.compare.dataset.join(', ');
  elements.learnSizeValue.textContent = elements.learnSize.value;
  elements.compareSizeValue.textContent = elements.compareSize.value;
  elements.speedLabel.textContent = speedToLabel(Number(elements.speedControl.value));
  state.learn.speed = Number(elements.speedControl.value);
  state.compare.speed = Number(elements.speedControl.value);
  applyAlgorithmMetaToLearn();
  renderBars(elements.learnChart, state.learn.dataset, null, 'single');
  renderBars(elements.algoAChart, state.compare.dataset, null, 'comparison');
  renderBars(elements.algoBChart, state.compare.dataset, null, 'comparison');
  elements.compareSummary.textContent = 'Run a comparison to see the result';
  elements.comparePause.textContent = 'Pause';
  elements.algoAName.textContent = algorithmLabel(state.compare.algorithmA);
  elements.algoBName.textContent = algorithmLabel(state.compare.algorithmB);
  elements.algoALabel.textContent = 'Left side';
  elements.algoBLabel.textContent = 'Right side';
}

async function bootstrap() {
  const ids = [
    'themeToggle', 'backendStatus', 'goLearn', 'goCompare', 'backFromLearn', 'backFromCompare',
    'learnAlgorithm', 'learnSize', 'speedControl', 'learnSizeValue', 'speedLabel', 'learnGenerate',
    'learnRun', 'learnPause', 'learnRestart', 'learnStepBack', 'learnStepForward', 'learnPseudocode',
    'learnCode', 'learnExplanation', 'learnComplexityBadge', 'learnStableBadge', 'learnComplexityRow',
    'learnChart', 'learnDatasetLabel', 'learnStepLabel', 'compareAlgorithmA', 'compareAlgorithmB',
    'manualDataset', 'compareSize', 'compareSizeValue', 'loadSampleDataset', 'csvInput', 'generateComparison',
    'runComparison', 'comparePause', 'compareSummary', 'algoAName', 'algoBName', 'algoAMetrics', 'algoBMetrics', 'algoAChart',
    'algoBChart', 'algoALabel', 'algoBLabel', 'algoAStepLabel', 'algoBStepLabel'
  ];

  ids.forEach((id) => {
    elements[id] = $(id);
  });

  setTheme(state.theme);
  initSelects();
  initDefaults();
  wireEvents();

  try {
    const health = await fetchJson(`${apiBase}/api/health`);
    updateBackendStatus(`${health.service} connected`);
  } catch (error) {
    updateBackendStatus('Backend not running yet', false);
  }
}

document.addEventListener('DOMContentLoaded', bootstrap);
