
API Reference
=============


.. autofunction:: sqsgenerator.public.process_settings

.. autoclass:: sqsgenerator.core.Structure
    :members: numbers, symbols, num_unique_species, unique_species, to_dict sorted, slice_with_species, with_species
    :undoc-members:

    .. property:: num_atoms

        The number of atoms in the structure

        :return: the number of atoms
        :rtype: int

    .. property:: distance_matrix

        The distance matrix with dimensions {num_atoms} times {num_atoms}

        :return: the distance matrix
        :rtype: ``np.ndarray``

    .. property:: distance_vecs

        The distance vectors taking into account periodic boundary conditions between the lattice positions.
        The distance matrix is computed from this property.

        :return: the vector of of shortest distances
        :rtype: ``np.ndarray``

    .. property:: frac_coords

        A ``np.ndarray`` storing the position of the lattice position in **fractional space**.

        :return: the fractional coordinates
        :rtype: ``np.ndarray``

.. autofunction:: sqsgenerator.public.make_supercell


.. autofunction:: sqsgenerator.public.make_result_document

.. autofunction:: sqsgenerator.public.pair_sqs_iteration
