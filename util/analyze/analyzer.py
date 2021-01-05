import csv
import sys
import functools
import copy


class Analyzer:
    POSITIONAL = [
        ('logs', 'The logs to analyze')
    ]

    # Documentation for any options
    OPTIONS = {}

    def __init__(self, **options):
        subclass = type(self).__name__
        assert (type(self).run is not Analyzer.run
                or type(self).run_bench is not Analyzer.run_bench), \
            f'Subclass `{subclass}` needs to implement run() or run_bench()'

        self.stats = [{'benchmark': 'Total'}]
        self.__current_benchmark = None
        self.options = options

    @property
    def current_benchmark(self):
        return self.__current_benchmark

    @current_benchmark.setter
    def current_benchmark(self, value):
        self.__current_benchmark = value
        self.stats.append({'benchmark': value})

    def run(self, args):
        # First, run for everything
        self.run_bench(args)

        # Then, run for each individual benchmark
        for benchmarks in zip(*args):
            assert all(it.name == benchmarks[0].name for it in benchmarks), \
                'Mismatching benchmark sequences'
            self.current_benchmark = benchmarks[0].name
            self.run_bench(benchmarks)

    def run_bench(self, args):
        raise NotImplementedError(
            f'Subclass `{type(self).__name__}` needs to implement run_bench()')

    def stat(self, *args, **kwargs):
        assert not args or not kwargs, \
            'Use named-argument syntax or function call syntax, but not both.'

        result = self.stats[-1]
        if args:
            name, value = args
            result[name] = value
        else:
            result.update(kwargs)

    def print_report(self, out):
        out = csv.DictWriter(out, fieldnames=self.stats[0].keys())
        out.writeheader()
        out.writerows(self.stats)


def analyze_all(*analyzer_classes):
    assert analyzer_classes
    if len(analyzer_classes) == 1:
        return analyzer_classes[0]

    return analyze_combined(
        *analyzer_classes,
        combiner=lambda s1, s2: [a.update(b) for a, b in zip(s1, s2)])


def analyze_combined(*analyzer_classes, combiner):
    assert len(analyzer_classes) >= 2
    assert all(len(cls.POSITIONAL) ==
               len(analyzer_classes[0].POSITIONAL) for cls in analyzer_classes)

    class CombinedAnalyzer(Analyzer):
        POSITIONAL = analyzer_classes[0].POSITIONAL
        OPTIONS = functools.reduce(lambda a, b: dict(
            a, **b), (cls.OPTIONS for cls in analyzer_classes))

        def __init__(self, **options):
            super().__init__(**options)
            self._analyzers = [cls(**options) for cls in analyzer_classes]

        def run(self, args):
            self.stats = None
            for analyzer in self._analyzers:
                analyzer.run(args)
                if not self.stats:
                    self.stats = copy.deepcopy(analyzer.stats)
                else:
                    combiner(self.stats, analyzer.stats)

    return CombinedAnalyzer
