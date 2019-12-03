__VERSION__ = 'v0.0.4'
__doc__ = """
sqsgenerator

Usage:
  sqsgenerator sqs <structure> <supercellx> <supercelly> <supercellz> [<composition>| --lattice=<SPECIES>...]
  [--verbosity=<VERBOSITY> --weights=<WEIGHTS> --output=<FILE> --iterations=<ITERATIONS> --parallel --objective=<OBJECTIVE> --format=<FORMAT>]
  sqsgenerator dosqs <structure> <supercellx> <supercelly> <supercellz> [<composition>| --lattice=<SPECIES>...]
  [--verbosity=<VERBOSITY> --weights=<WEIGHTS> --output=<FILE> --iterations=<ITERATIONS> --parallel --anisotropy=<ANISOTROPY> --format=<FORMAT>]
  sqsgenerator alpha sqs <structure> [--weights=<WEIGHTS> --verbosity=<VERBOSITY> --sublattice=<SUBLATTICE>...]
  sqsgenerator alpha dosqs <structure> [--weights=<WEIGHTS> --verbosity=<VERBOSITY> --anisotropy=<ANISOTROPY> --sublattice=<SUBLATTICE>...]
  sqsgenerator --help
  sqsgenerator --version

<structure>              The POSCAR, cif, cssr or Exciting input xml file which contains the structural information
<supercellx>             The amount of unitcells to be stacked into x direction
<supercelly>             The amount of unitcells to be stacked into y direction
<supercellz>             The amount of unitcells to be stacked into z direction
<composition>            The composition of the system as a list of mole fractions. It must contain n-1 or n values
                         separated with a comma. In a binary system e.g. B:0.5 in a ternary B:0.5,N:0.2. Where n is the
                         number of species specified in the POSCAR file. If n values are specified their sum must be 1.
                         You can specify a vacancy concentration by providing a mole fraction with species '0'. e.g A
                         Boron Nitride system with a vacancy concentration of 2% B:0.49,N:0.49,0 of or use the
                         appropriate switch

Options:
--output, -O=<FILE>              Limits the number of output files. The output file will be a zip-archive if equivalent
                                 structures are found. Otherwise only one file will be written. If less equivalent
                                 structures are found than specified with this switch only those will be written.
                                 If "all" is provided by the user all equivalent structures will be written to the ouput
                                 archive.
                                 NOTE: Use "all" only for small supercells and preferably with the "dosqs" command
                                 [default: 10]

--parallel, -P                   Flag for either making the computation parallel or not. Since this project is currently
                                 under development use this flag preferably with "-I all" to get optimal performance.

--lattice, -L=<SPECIES>          Specify the sublattice/s on which the sqsgen should run. At first specify the
                                 sublattice species followed by the compositions. For example to place Tantalum carbide
                                 on the nitrogen sites of a boron nitride system use N=Ta:0.8,C:0.2. To replace a specie
                                 just use B=Hf:1.0

--weights, -W=<WEIGHTS>          Specify the weights of the individual shells by a list of comma separated values. By
                                 default the shell weight is the inverse of the number e.g 1, 1/2, 1/3 ... All shells
                                 for which no value was specified are set to 0. The position in the list corresponds to
                                 the shell number. [default: 1,0.5,0.3333333333,0.25,0.2,0.166666667,0.1428571429]

--anisotropy, -A=<ANISOTROPY>    This option is only available when using the "dosqs" command. The first value specifies
                                 the weight of the sum of all alphas (alpha_x + alpha_y + alpha_z). The second value
                                 represents the weight of the difference of (alpha_x - alpha_y). The third for
                                 (alpha_x - alpha_z), thus the forth (alpha_y - alpha_z). You may specify either 1, 2 or
                                 4 values with:
                                 1 value:  Interpreted as the sum of all alphas. If the value is smaller than 1 the other
                                           three values are equal so that all 4 weights sum up to 1. If the value is
                                           greater than 1 the three weights are set to one. Values must be grater than 0
                                 2 values: The first value is interpreted as the sum of all alphas. The second the
                                           weight of (alpha_x - alpha_y). NOTE: Only the x and y direction are taken
                                           into consideration for the minimization.
                                 4 values: All values are interpreted as outlined above. All three directions are taken
                                           into account.
                                 The default values (when omitting this switch) depend on the material. If the material
                                 is hexagonal or tetragonal the programm will resume with 0.6,0.4 for cubic lattices
                                 0.4,0.2,0.2,0.2 will be used. [default: 0.4,0.2,0.2,0.2]

--iterations, -I=<ITERATIONS>    Perform a definite amount of iterations. If the value is a number is a number
                                 a random shuffling approach will be used for generating structures. If "all" is specified
                                 a systematic lexicographical approach will be used for structure generation.
                                 WARNING: Do not use for big supercells > 32-36 atoms [default: 100000]

--verbosity=<VERBOSITY>          Sets the level of verbosity. Values from 1-5 are possible [default: 1]

--objective=<OBJECTIVE>          Specifies the value the objective functions. The program tries to reach the specified
                                 objective function. [default: 0.0]
                                 
--sublattice, -S=<SUBLATTICE>    Specify a sublattice using the original structure file form which the system, which is
                                 to be analyzed was created from. --sublattice=/path/to/orig_structure,2,2,2,Ga:Fe [default:]
                                 
--format, -F=<FORMAT>            Specifies the output file format. Currently "cif", "lammps", "cssr", and "vasp"
                                 is available. [default: vasp]

--version                        Displays the version of sqsgen

"""
import numpy as np


from pymatgen import Structure
from pymatgen.io.vasp import Poscar
from sqsgenerator.utils.docopt import docopt
from sqsgenerator.utils import write_message, unicode_alpha, get_superscript, colored, DEBUG, unicode_capital_sigma
from sqsgenerator.utils.optionparser import parse_options


def main():
    options = docopt(__doc__, version=__VERSION__)
    options = parse_options(options)
    if options['alpha']:
        if options['sublattice']:
            for s in options['sublattice']:
                calculate_alpha(options, s)
    else:
        structure = options['structure']
        structure.make_supercell([options[k] for k in ['supercellx', 'supercelly', 'supercellz']])

        if options['lattice']:
            structures = sublattice_iterations(options)
        else:
            structures = default_iterations(options)

        fname = '{x}x{y}x{z}'.format(x=options['supercellx'], y=options['supercelly'], z=options['supercellz'])
        write_structures(structures, fname, options['format'])


def calculate_alpha(options, structure):
    atoms = len(structure.sites)
    species = list(set([element.symbol for element in structure.species]))
    mole_fractions = {}
    # Compute mole fraction dictionary of given structure
    for specie in species:
        mole_fractions[specie] = float(
            len(
                list(
                    filter(lambda site: site.specie.name == specie, structure.sites)
                )
            )
        ) / atoms
    if options['sqs']:
        from sqsgenerator.core.sqs import SqsIterator
        iterator = SqsIterator(structure, mole_fractions, options['weights'], verbosity=options['verbosity'])
        alpha = iterator.calculate_alpha()
    elif options['dosqs']:
        from sqsgenerator.core.dosqs import DosqsIterator
        iterator = DosqsIterator(structure, mole_fractions, options['weights'], verbosity=options['verbosity'])
        main_sum_weight, anisotropy_weights = options['anisotropy']
        alpha = iterator.calculate_alpha(main_sum_weight, anisotropy_weights)
    else:
        write_message('An unexpected error occurred')
    print_result(options, alpha, options['verbosity'])

def do_sqs_iterations(structure, mole_fractions, weights, iterations=10000, prefix='', verbosity=0, parallel=True, output_structures=10, objective=0.0):
    """
    Performs a the iteration by generating random arrangements of the atoms.

    Args:
        structure (:class:`pymatgen.Structure`): A structure object from the pymatgen project, which defines the atom sites
        mole_fractions (dict): A dictionary specifying the composition. Keys are the species element symbol.
            The values represent the corresponding mole fraction e.g::

                mole_fractions = {
                    'Ta': 0.8,
                    'Nb': 0.1,
                    'C': 0.05,
                    '0': 0.05
                }
        weights (dict): The weights for the individual shells as described in :func:`core.calculate_matrix`
        iterations (float): The number of iterations to be done (default: 10000)
        parallel (bool): A flag for indicating parallel computation
        prefix (str): A string which is put before any output of this method. Intended usage is to mark sublattice
            generations

        Returns:
            alpha (float): The minimum alpha that was found
            configuration (list): The corresponding configuration

    """
    print("{3}SQS Iteration input:\n"
          "{3}====================\n"
          "{3}Iterations: {0}\n"
          "{3}Composition: {1}\n"
          "{3}Weighting: {2}\n"
          "{3}====================".format(iterations, mole_fractions, weights, prefix))

    if not parallel:
        from sqsgenerator.core.sqs import SqsIterator
        iterator = SqsIterator(structure, mole_fractions, weights, verbosity=verbosity)
    else:
        from sqsgenerator.core.sqs import ParallelSqsIterator
        iterator = ParallelSqsIterator(structure, mole_fractions, weights, verbosity=verbosity)

    structures, decmp, iter_, cycle_time = iterator.iteration(iterations=iterations, output_structures=output_structures, objective=objective)

    print("{1}Needed {0:.2f} microsec per permutation".format(cycle_time * 1e6, prefix))
    return structures, decmp, iter_


def do_dosqs_iterations(structure, mole_fractions, weights, sum_weight, anisotropic_weights, iterations=10000,
                        prefix='', verbosity=0, parallel=False, output_structures=10):
    header = """
    {prefix}Direction optimized SQS Iteration input:
    {prefix}========================================
    {prefix}Iterations: {iterations}
    {prefix}Composition: {composition}
    {prefix}Shell weighting: {shell_weights}
    {prefix}Weight {unicode_capital_sigma}({unicode_alpha}[x] + {unicode_alpha}[y] + {unicode_alpha}[z]): {main_sum_weight}
    {prefix}Weight ({unicode_alpha}[x] - {unicode_alpha}[y]): {alpha_xy}
    {prefix}Weight ({unicode_alpha}[x] - {unicode_alpha}[z]): {alpha_xz}
    {prefix}Weight ({unicode_alpha}[y] - {unicode_alpha}[z]): {alpha_yz}
    {prefix}========================================
    """.format(iterations=iterations,
               composition=mole_fractions,
               shell_weights=weights,
               main_sum_weight=sum_weight,
               alpha_xy=anisotropic_weights[0],
               alpha_xz=anisotropic_weights[1],
               alpha_yz=anisotropic_weights[2] if len(anisotropic_weights) == 3 else 0.0,
               prefix=prefix,
               unicode_alpha=unicode_alpha,
               unicode_capital_sigma=unicode_capital_sigma)
    print(header)
    if not parallel:
        from sqsgenerator.core.dosqs import DosqsIterator
        iterator = DosqsIterator(structure, mole_fractions, weights, verbosity=verbosity)
    else:
        from sqsgenerator.core.dosqs import ParallelDosqsIterator
        iterator = ParallelDosqsIterator(structure, mole_fractions, weights, verbosity=verbosity)

    structures, decmp, iter_, cycle_time = iterator.iteration(sum_weight, anisotropic_weights, iterations=iterations, output_structures=output_structures)
    print("{1}Needed {0:.2f} microsec per permutation".format(cycle_time * 1e6, prefix))
    return structures, decmp, iter_


def print_result_internal(alpha, verbosity, options, prefix=''):
        if verbosity > 0:
            sum_alpha = 0
            for bond, values in alpha.items():
                sum_alpha += sum([abs(v) for v in values])
            print(colored('{alpha}{prefix}={value}'.format(alpha=unicode_alpha,
                                                           value=sum_alpha,
                                                           prefix=prefix,), color='magenta'))
        if 1 <= verbosity < 2:
            shell_alphas = np.sum(np.array(list(alpha.values())),axis=0).tolist()
            for i, shell_alpha in enumerate(shell_alphas):
                print(colored('{alpha}{prefix}{sub}={value:.14f}'.format(alpha=unicode_alpha,
                                                            sub=get_superscript(i+1),
                                                            prefix=prefix,
                                                            value=shell_alpha), color='magenta'))
        if 2 <= verbosity <= 3:
            for bond, values in alpha.items():
                print(colored('{alpha}{prefix}[{bond}]={value:.14f}'.format(alpha=unicode_alpha,
                                                               bond=bond,
                                                               prefix=prefix,
                                                               value=sum(values)), color='magenta'))
        if verbosity > 3:
            for bond, values in alpha.items():
                line_str = ""
                for i, value in enumerate(values[:-1]):
                    line_str += colored('{alpha}{sub}[{bond}]={value:.14f}, '.format(
                        alpha=unicode_alpha,
                        bond=bond,
                        sub=get_superscript(i+1),
                        value=value
                    ), color='magenta')
                line_str += colored('{alpha}{sub}[{bond}]={value:.14f}'.format(
                        alpha=unicode_alpha,
                        bond=bond,
                        sub=get_superscript(len(values)),
                        value=values[-1]
                    ), color='magenta')
                print(line_str)


def print_result(options, alpha, verbosity):
    if options['sqs']:
        print_result_internal(alpha, verbosity, options)
    if options['dosqs']:
        print_result_internal(alpha['x'], verbosity, options, prefix='{x}')
        print()
        print_result_internal(alpha['y'], verbosity, options, prefix='{y}')
        print()
        print_result_internal(alpha['z'], verbosity, options, prefix='{z}')


def write_structures(structures, file_name, format):
    import tempfile
    import zipfile
    from os.path import basename, join
    import os
    format_name, Writer = format
    file_type = format_name
    if len(structures) > 1:
        result_archive = zipfile.ZipFile(join(os.getcwd(), basename(file_name) + '.zip'), mode='w')
        for name , structure in structures.items():
            sorted_sites = list(sorted(structure.sites.copy(), key=lambda site: site.specie.symbol))
            sorted_structure = Structure.from_sites(sorted_sites)
            with tempfile.NamedTemporaryFile() as tmpfile:
                Writer(sorted_structure).write_file(tmpfile.name)
                result_archive.write(tmpfile.name, arcname='{}.{}'.format(name, file_type))
        result_archive.close()
        write_message('Archive file: {0}'.format(join(os.getcwd(), basename(file_name) + '.zip')), level=DEBUG)
    else:
        #There is just one file write only that one
        structure_key = list(structures.keys())[0]
        sorted_sites = list(sorted(structures[structure_key].sites.copy(), key=lambda site: site.specie.symbol))
        sorted_structure = Structure.from_sites(sorted_sites)
        p = Writer(sorted_structure)
        fname = join(os.getcwd(), '{}.{}'.format(structure_key, file_type))
        p.write_file(fname)
        write_message('Output file: {0}'.format(fname), level=DEBUG)


def default_iterations(options):
    structure = options['structure']
    if options['sqs']:
        structures, decompositions, iterations = do_sqs_iterations(structure,
                                                                            options['composition'],
                                                                              options['weights'],
                                                                              iterations=options['iterations'],
                                                                              verbosity=options['verbosity'],
                                                                              parallel=options['parallel'],
                                                                              output_structures=options['output'],
                                                                              objective=options['objective'])
        print_result(options, decompositions[0], verbosity=options['verbosity'])
    elif options['dosqs']:
        main_sum_weight, anisotropy_weights = options['anisotropy']
        structures, decompositions, iterations = do_dosqs_iterations(structure,
                                                   options['composition'],
                                                   options['weights'],
                                                   main_sum_weight,
                                                   anisotropy_weights,
                                                   iterations=options['iterations'],
                                                   verbosity=options['verbosity'],
                                                   parallel=options['parallel'],
                                                   output_structures=options['output'])
        print_result(options, decompositions[0], verbosity=options['verbosity'])

    result = {}
    for i, structure in enumerate(structures):
        spec_set = tuple(sorted(tuple(set([site.specie.symbol for site in structure.sites]))))
        name = '{0}-{1}'.format(i, ''.join(spec_set))
        result[name] = structure
    return result


def sublattice_iterations(options):
    sublattice_composition = options['lattice']
    structure = options['structure']
    sublattice_species = list(sublattice_composition.keys())
    remaining_site_collection = list(filter(lambda site: site.specie.symbol not in sublattice_species, structure.sites))

    structures = {}
    sublattice_structure_mapping = {}
    for sublattice, mole_fractions in sublattice_composition.items():
        sublattice_site_collection = list(filter(lambda site: site.specie.symbol == sublattice, structure.sites))
        # initial_structure = structure
        sublattice_structure = Structure(structure.lattice,
                                         [site.specie for site in sublattice_site_collection],
                                         [site.frac_coords for site in sublattice_site_collection])
        if options['sqs']:
            sublattice_structures, decompositions, iterations = do_sqs_iterations(sublattice_structure,
                                                                                  mole_fractions,
                                                                                  options['weights'],
                                                                                  iterations=options['iterations'],
                                                                                  prefix=colored('{0} => '.format(sublattice), color='magenta'),
                                                                                  verbosity=options['verbosity'],
                                                                                  parallel=options['parallel'],
                                                                                  output_structures=options['output'],
                                                                                  objective=options['objective'])
            print_result(options, decompositions[0], options['verbosity'])
        elif options['dosqs']:
            main_sum_weight, anisotropy_weights = options['anisotropy']
            sublattice_structures, decompositions, iterations = do_dosqs_iterations(sublattice_structure,
                                                       mole_fractions,
                                                       options['weights'],
                                                       main_sum_weight,
                                                       anisotropy_weights,
                                                       iterations=options['iterations'],
                                                       prefix=colored('{0} => '.format(sublattice),
                                                                    color='magenta'),
                                                       verbosity=options['verbosity'],
                                                       parallel=options['parallel'],
                                                       output_structures=options['output'])
            print_result(options, decompositions[0], options['verbosity'])
        #Merge both two sublattices
        #map sites to collections
        sublattice_structure_mapping[sublattice] = sublattice_structures
        write_message('{0} structures found on sublattice {1}'.format(len(sublattice_structures), sublattice), level=DEBUG)

    structure_list = []
    for sl, sc in sublattice_structure_mapping.items():
        for i, s in enumerate(sc):
            structure_list.append((i,sl))
    from itertools import combinations

    strc_count = 0
    for i, combination in enumerate(combinations(structure_list, len(sublattice_structure_mapping))):
        subl_spec = [tup[1] for tup in combination]
        if len(set(subl_spec)) == len(subl_spec):
            collection = remaining_site_collection.copy()
            name = '{0}'.format(strc_count)
            for j, sl in combination:
                structure = sublattice_structure_mapping[sl][j]
                spec_set = set([site.specie.symbol for site in structure.sites])
                collection.extend([site for site in structure.sites])
                name += '-{1}-{0}'.format(''.join(spec_set), j)
            structures[name] = Structure.from_sites(collection)
            strc_count += 1

    return structures


if __name__ == '__main__':
    main()