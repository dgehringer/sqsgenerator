
import yaml
import unittest
from typing import Set
from sqsgenerator import sqs_optimize
from sqsgenerator.public import SQSCallback, SQSCoreCallback, rank_structure

with open('resources/ti-al-b-n.yaml', 'r') as fh:
    config = yaml.safe_load(fh)


def make_inserter(d: Set[int]) -> SQSCoreCallback:
    def _cb(_, result, *__):
        d.add(result.rank)

    return _cb


class TestOptimizationCallbacks(unittest.TestCase):

    def test_callbacks(self):
        ranks_better_or_equal = set()
        ranks_better = set()

        callbacks = dict(
            better_or_equal=[make_inserter(ranks_better_or_equal)],
            better=[make_inserter(ranks_better)]
        )

        settings = config.copy()
        settings.update(callbacks=callbacks)
        sqs_optimize(settings)
        self.assertTrue(ranks_better.issubset(ranks_better_or_equal))


    def test_callbacks_and_rank_structure(self):
        ranks_better_or_equal = set()
        ranks_better = set()

        def make_inserter_and_rank(d: Set[int]) -> SQSCallback:
            def _cb(_, structure, result, *__):
                d.add(result.rank)
                self.assertTrue(rank_structure(structure) == result.rank)
            return _cb

        callbacks = dict(
            better_or_equal=[make_inserter_and_rank(ranks_better_or_equal)],
            better=[make_inserter_and_rank(ranks_better)]
        )

        settings = config.copy()
        settings.update(callbacks=callbacks)
        sqs_optimize(settings, pass_structure=True)
        self.assertTrue(ranks_better.issubset(ranks_better_or_equal))

