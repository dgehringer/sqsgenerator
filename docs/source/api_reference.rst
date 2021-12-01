
API Reference
=============

Most important functions
########################

.. autofunction:: sqsgenerator.public.sqs_optimize

.. autofunction:: sqsgenerator.public.sqs_analyse

Helper functions
################

.. autofunction:: sqsgenerator.public.process_settings

.. autofunction:: sqsgenerator.public.read_settings_file

.. autofunction:: sqsgenerator.public.export_structures

.. autofunction:: sqsgenerator.public.make_rank

.. autofunction:: sqsgenerator.public.total_permutations

.. autofunction:: sqsgenerator.public.rank_structure

Conversion functions
####################

.. autofunction:: sqsgenerator.public.to_ase_atoms

.. autofunction:: sqsgenerator.public.from_ase_atoms

.. autofunction:: sqsgenerator.public.to_pymatgen_structure

.. autofunction:: sqsgenerator.public.from_pymatgen_structure


low-level API reference
#######################

.. autoclass:: sqsgenerator.public.IterationMode

    The IterationMode is a simple enum class which indicates how ``sqsgenerator`` should sample the configurational space

    .. attribute:: random

        Instructs the program to use a Monte-Carlo based sampling of the configurational space

    .. attribute:: systematic

        Will cause sqsgenerator to systematically sample the configurational space. This setting is recommended only
        for relatively "*small*" structures. We strongly encourage you to use:

            .. highlight:: bash
            .. code-block:: bash

                sqsgen compute total-permutations

        to obtain the potential number of iterations needed. To roughly estimate the time on your local machine
        you can use:

            .. highlight:: bash
            .. code-block:: bash

                sqsgen compute estimated-time

.. autoclass:: sqsgenerator.public.Structure
    :members: numbers, symbols, num_unique_species, unique_species, to_dict, sorted, slice_with_species, with_species
    :undoc-members:

    .. autofunction:: sqsgenerator.public.Structure.__init__

    .. property:: num_atoms

        The number of atoms in the structure

        :return: the number of atoms
        :rtype: int

    .. property:: distance_matrix

        The distance matrix with dimensions {num_atoms} times {num_atoms}

        :return: the distance matrix
        :rtype: np.ndarray

    .. property:: distance_vecs

        The distance vectors taking into account periodic boundary conditions between the lattice positions.
        The distance matrix is computed from this property.

        :return: the vector of of shortest distances
        :rtype: np.ndarray

    .. property:: frac_coords

        A ``np.ndarray`` storing the position of the lattice position in **fractional space**.

        :return: the fractional coordinates
        :rtype: np.ndarray

.. autofunction:: sqsgenerator.public.make_supercell

.. autofunction:: sqsgenerator.public.make_result_document

.. autofunction:: sqsgenerator.public.pair_sqs_iteration

.. autofunction:: sqsgenerator.public.extract_structures

.. autofunction:: sqsgenerator.public.expand_sqs_results

.. autoclass:: sqsgenerator.public.SQSResult

