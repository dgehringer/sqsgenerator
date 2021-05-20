import re

from .utils import write_message, full_name, ERROR, WARNING, parse_separated_string, parse_float, all_subclasses, \
    ilen, INFO
from pymatgen.core.periodic_table import Element
from os.path import exists, isfile, basename
from math import isclose
from pymatgen.core.structure import Structure


def parse_options(docopt_options):
    options = {}
    exceptions = [CompositionalArgument]
    parsers = {subcls(docopt_options).format_key(): subcls(docopt_options) for subcls in all_subclasses(ArgumentBase) if
               subcls not in exceptions}
    for key in docopt_options.keys():
        if key.startswith('--') or (key.startswith('<') and key.endswith('>')):
            if docopt_options[key] is None:
                continue
            if key not in parsers:
                write_message('No suitable parser found for argument {0}'.format(key), exit=True)
            options[parsers[key].key] = parsers[key]()
        else:
            options[key] = docopt_options[key]
    return options


class Singleton(object):
    # the one, true Singleton
    __single = None

    def __new__(classtype, *args, **kwargs):
        # Check to see if a __single exists already for this class
        # Compare class types instead of just looking for None so
        # that subclasses will create their own __single objects
        if classtype != type(classtype.__single):
            classtype.__single = object.__new__(classtype)
            classtype.__single.__init__(*args, **kwargs)
        return classtype.__single


class InvalidOption(Exception):
    """
    Arguments vor options are invalid
    """


class ArgumentBase(Singleton):

    def __init__(self, options, key, option=False):
        self._options = options
        self._key = key
        self._parsed = False
        self._result = None
        self._option = option

    @property
    def key(self):
        return self._key

    def format_key(self):
        return ('--{key}' if self._option else '<{key}>').format(key=self._key)

    @property
    def raw_value(self):
        return self._options[self.format_key()]

    def parse(self, options, *args, **kwargs):
        return options[self.format_key()]

    def __call__(self, *args, **kwargs):
        if not self._parsed:
            try:
                self._result = self.parse(self._options, args, kwargs)
            except InvalidOption:
                write_message(
                    '{name}: Failed to parse "{arg}" for ' + 'argument' if not self._option else 'option' + ' "{option}"'.format(
                        name=full_name(self), option=self.key, arg=self.raw_value, exit=True))
            else:
                self._parsed = True
        return self._result

    def get_value(self, other, *args, **kwargs):
        return other(self._options, *args, **kwargs)()

    def write_message(self, message, level=ERROR, exit=False):
        write_message('{name}: {message}'.format(name=full_name(self), message=message), level=level, exit=exit)


class CompositionalArgument(ArgumentBase):

    def __init__(self, options, key, option=False):
        super(CompositionalArgument, self).__init__(options, key, option=option)
        self._composition_regex = re.compile(r"([0A-Z][a-z]?)(?=:|$)(:)?(0?\.\d+|\d+)?")
        self._sx, self._sy, self._sz = map(self.get_value, [
            SupercellXArgument,
            SupercellYArgument,
            SupercellZArgument
        ])

    @staticmethod
    def is_valid_species(s: str):
        return Element.is_valid_symbol(s) or s == "0"

    def parse_composition(self, composition, species, atoms):
        matches = list(map(self._composition_regex.match, parse_separated_string(composition)))
        # check if we can parse all parts
        if any(m is None for m in matches):
            self.write_message(f"Invalid composition \"{composition}\"", level=ERROR)
            raise InvalidOption
        matches = [m.groups() for m in matches] # retrieve groups from matches
        mole_fractions = {element: parse_float(amount) for element, _, amount in matches}
        # one species is allowed to be given no composition, since we always fill up to 1
        missing_species = [element for element, amount in mole_fractions.items() if amount is None]
        atoms_in_supercell = atoms * self._sx * self._sy * self._sz

        # if any of those atomic fractions is larger than one and an integer value we, think that the
        # user wants to specify the composition in atoms rather than in fractions
        atom_mode = any(
            fraction.is_integer() and fraction >= 1
            for fraction in mole_fractions.values()
            if fraction is not None
        )
        # if we are in atom mode we have to rescale the number of atoms by the total number of atoms in the cell
        if atom_mode:
            self.write_message(f"Assuming atom style composition input mode: Rescaling by {atoms_in_supercell}", level=INFO)
            mole_fractions = {
                element: amount if amount is None else amount/atoms_in_supercell
                for element, amount in mole_fractions.items()
            }

        # if more than one species is undetermined we cannot continue
        if len(missing_species) > 1:
            self.write_message('You did not specify enough species. '
                               'I\'m missing {0}!'.format(missing_species), level=ERROR)
            raise InvalidOption
        elif len(missing_species) == 1:
            # in this case we are sure that our mole_fractions sum up to one
            missing_amount = 1.0 - sum(amount for amount in mole_fractions.values() if amount is not None)
            missing_specie = missing_species[0]
            mole_fractions[missing_specie] = missing_amount
        else:
            # missing_species == 0 everything is here
            assigned_fraction = sum(mole_fractions.values())
            # as tolerance we take half an atom
            if not isclose(assigned_fraction, 1, abs_tol=0.5/atoms_in_supercell):
                self.write_message(f"The mole fractions/number of atoms do not sum up to 1/{atoms_in_supercell}", level=ERROR)
                raise InvalidOption

        return mole_fractions

    def parse(self, options, *args, **kwargs):
        raise NotImplementedError


class LatticeOption(CompositionalArgument):

    def __init__(self, options):
        super(LatticeOption, self).__init__(options, 'lattice', option=True)

    def parse(self, options, *args, **kwargs):
        structure = StructureFileArgument(options)()
        species = list(set([element.symbol for element in structure.species]))
        sublattice_composition = {}
        sublattice_mapping = self.raw_value
        for mapping in sublattice_mapping:
            try:
                sublattice, composition = mapping.split('=')
            except:
                self.write_message('Could not parse sublattice definition "{0}"'.format(mapping))
                raise InvalidOption
            sublattice_species = []
            dummy_z = 1
            for sublattice_composition_string in parse_separated_string(composition):

                try:
                    _species, mole_fraction = sublattice_composition_string.split(':')
                except ValueError:
                    if Element.is_valid_symbol(
                            sublattice_composition_string) or sublattice_composition_string.startswith('0'):
                        _species = sublattice_composition_string
                    else:
                        _species = Element.from_Z(dummy_z).symbol
                        dummy_z += 1
                sublattice_species.append(_species)

            for _species in sublattice_species:
                if not Element.is_valid_symbol(_species) and not _species == '0':
                    self.write_message(
                        'Element "{0}" is not a valid symbol in the periodic table. To get a list of all symbols you may have a look at pymatgen.core.periodic_table'.format(
                            _species))
                    raise InvalidOption

            if sublattice == '0':
                self.write_message('Vacancies cannot be specified as sublattice'.format(sublattice))
                raise InvalidOption
            elif sublattice not in species:
                self.write_message('"{0}" is not a specified species'.format(sublattice))
                raise InvalidOption
            if len(sublattice_species) == 0:
                self.write_message(
                    'You must specify which species should be placed on the {0} sublattice'.format(sublattice))
                raise InvalidOption
            sublattice_composition[sublattice] = \
                self.parse_composition(composition, sublattice_species,
                                       atoms=self.get_atom_number_on_sublattice(sublattice, structure))

        return sublattice_composition

    @staticmethod
    def get_atom_number_on_sublattice(species, structure):
        return ilen(filter(lambda site: site.specie.symbol == species, structure.sites))


class WeightsOption(ArgumentBase):

    def __init__(self, options):
        super(WeightsOption, self).__init__(options, key='weights', option=True)

    def parse(self, options, *args, **kwargs):
        try:
            weight_values = list(map(lambda str_: parse_float(str_, except_val=0, raise_exc=True),
                                     parse_separated_string(options[self.format_key()])))

            weights = dict(zip(range(1, len(weight_values) + 1), weight_values))
        except ValueError:
            self.write_message('An error occurred while parsing weights. Exiting!\n')
            raise InvalidOption
        else:
            return weights


class ObjectiveOption(ArgumentBase):

    def __init__(self, options):
        super(ObjectiveOption, self).__init__(options, key='objective', option=True)

    def parse(self, options, *args, **kwargs):
        if self.raw_value.lower().startswith('i'):
            objective_value = float('inf')
        elif self.raw_value.lower().startswith('-i'):
            objective_value = float('-inf')
        else:
            try:
                objective_value = float(options[self.format_key()].lower())
            except ValueError:
                self.write_message(
                    'Cannot parse objective function value: "{0}" . Exiting!\n'.format(options[self.format_key()]))
                raise InvalidOption
        return objective_value


class AnisotropyOption(ArgumentBase):

    def __init__(self, options):
        super(AnisotropyOption, self).__init__(options, key='anisotropy', option=True)

    def parse(self, options, *args, **kwargs):
        try:
            anisotropy_values = list(map(lambda str_: parse_float(str_, except_val=0, raise_exc=True),
                                         parse_separated_string(self.raw_value)))
            if len(anisotropy_values) not in (1, 2, 4):
                self.write_message("Only 1, 2 or 4 values can be specified for the directional weighting. "
                                   "Use the -h switch for help")
                raise InvalidOption

            main_sum_weight = anisotropy_values[0]
            if len(anisotropy_values) == 1:
                anisotropy_weights = [1.0] * 3 if main_sum_weight >= 1.0 else [(1 - main_sum_weight) / 3] * 3
            else:
                anisotropy_weights = anisotropy_values[1:]
        except ValueError:
            self.write_message('An error occurred while parsing anisotropy weights. Exiting!\n')
            raise InvalidOption
        else:
            return main_sum_weight, anisotropy_weights


class ParallelOption(ArgumentBase):

    def __init__(self, options):
        super(ParallelOption, self).__init__(options, key='parallel', option=True)


class VerbosityOption(ArgumentBase):

    def __init__(self, options):
        super(VerbosityOption, self).__init__(options, key='verbosity', option=True)

    def parse(self, options, *args, **kwargs):
        try:
            verbosity = int(parse_float(self.raw_value, raise_exc=True))
        except ValueError:
            self.write_message('Could not parse verbosity level')
            raise InvalidOption
        else:
            return verbosity


class VacancyOption(ArgumentBase):

    def __init__(self, options):
        super(VacancyOption, self).__init__(options, key='vacancy', option=True)

    def parse(self, options, *args, **kwargs):
        # The vacancy switch
        try:
            mole_fraction_vacancy = abs(parse_float(self.raw_value, raise_exc=True))
        except:
            self.write_message('Could not parse vacancy definition')
            raise InvalidOption
        else:
            return mole_fraction_vacancy


class IterationsOption(ArgumentBase):

    def __init__(self, options):
        super(IterationsOption, self).__init__(options, key='iterations', option=True)

    def parse(self, options, *args, **kwargs):
        try:
            iterations = int(parse_float(self.raw_value, raise_exc=True))
        except ValueError:
            if self.raw_value.lower() == 'all':
                return self.raw_value
            else:
                self.write_message('Could not parse iteration number')
                raise InvalidOption
        else:
            return iterations


class OutputOption(ArgumentBase):

    def __init__(self, options):
        super(OutputOption, self).__init__(options, key='output', option=True)

    def parse(self, options, *args, **kwargs):
        try:
            output = int(parse_float(self.raw_value, raise_exc=True))
        except ValueError:
            if self.raw_value.lower() == 'all':
                return self.raw_value
            else:
                self.write_message('Could not output structure number')
                raise InvalidOption
        else:
            return output


class HelpOption(ArgumentBase):

    def __init__(self, options):
        super(HelpOption, self).__init__(options, key='help', option=True)

    def parse(self, options, *args, **kwargs):
        import sqsgenerator.cli
        return sqsgenerator.cli.__doc__


class VersionOption(ArgumentBase):

    def __init__(self, options):
        super(VersionOption, self).__init__(options, key='version', option=True)

    def parse(self, options, *args, **kwargs):
        from sqsgenerator.cli import __VERSION__
        return __VERSION__


class FormatOption(ArgumentBase):

    def __init__(self, options):
        super(FormatOption, self).__init__(options, key='format', option=True)
        from pymatgen.io.vasp import Poscar
        from pymatgen.io.cif import CifWriter
        from pymatgen.io.cssr import Cssr
        from pymatgen.io.lammps.inputs import LammpsData

        def _lammps_wrapper(structure):
            return LammpsData.from_structure(structure, atom_style='atomic')

        self._class_mapping = {
            'vasp': Poscar,
            'lammps': _lammps_wrapper,
            'cssr': Cssr,
            'cif': CifWriter
        }

    def parse(self, options, *args, **kwargs):
        if self.raw_value not in self._class_mapping:
            raise InvalidOption('Output format must be in {}'.format(self._options))
        return self.raw_value, self._class_mapping[self.raw_value]


class StructureFileArgument(ArgumentBase):

    def __init__(self, options):
        super(StructureFileArgument, self).__init__(options, key='structure')

    def parse(self, options, *args, **kwargs):
        path = self.raw_value
        if exists(path):
            if isfile(path):
                filename = basename(path)
                if filename.lower().endswith('.cif'):
                    from pymatgen.io.cif import CifParser
                    try:
                        result = CifParser(path).get_structures()[0]
                        format = 'cif'
                    except:
                        self.write_message('Could not parse CIF file: "{0}"'.format(path))
                        raise InvalidOption
                elif filename.lower().endswith('.cssr'):
                    from pymatgen.io.cssr import Cssr
                    try:
                        result = Cssr.from_file(path).structure
                        format = 'cssr'
                    except:
                        self.write_message('Could not parse CSSR file: "{0}"'.format(path))
                        raise InvalidOption
                else:
                    from pymatgen.io.vasp import Poscar
                    try:
                        result = Poscar.from_file(path).structure
                        fromat = 'vasp'
                    except:
                        self.write_message('Could not parse POSCAR file "{0}"'.format(path))
                        raise InvalidOption
                if not 'result' in locals():
                    write_message('File format not supported')
                    raise InvalidOption
                else:
                    return result
            else:
                self.write_message('"{0}" is a directory!'.format(path))
                raise InvalidOption
        else:
            self.write_message('Cannot locate file "{0}"'.format(path))
            raise InvalidOption


class CompositionArgument(CompositionalArgument):

    def __init__(self, options):
        super(CompositionArgument, self).__init__(options, key='composition')

    def parse(self, options, *args, **kwargs):
        structure = StructureFileArgument(options)()
        species = list(set([element.symbol for element in structure.species]))
        # Parse the given composition
        mole_fractions = self.parse_composition(self.raw_value, species, len(structure.sites))
        vacancy_parser = VacancyOption(options)
        if vacancy_parser.format_key() in options and options[vacancy_parser.format_key()]:
            mole_fraction_vacancy = vacancy_parser()
            # Check if given value is sensible
            if isclose(mole_fraction_vacancy, 1.0) or 0.0 >= mole_fraction_vacancy > 1.0:
                self.write_message('Vacancy mole fraction cannot be 1 or greater or less or equal to 0')
                raise InvalidOption
            # Vacancy species already specified by -S --species switch
            if '0' in mole_fractions:
                mole_fractions['0'] = 0.0
                # Correction factor for the other mole fractions
                correction_factor = 1 / sum(mole_fractions.values())
                # Correct other mole fractions
                for _species, mole_fraction in mole_fractions.items():
                    mole_fractions[_species] = mole_fraction * correction_factor

            correction_factor = (1.0 - mole_fraction_vacancy) / sum(mole_fractions.values())
            for _species, mole_fraction in mole_fractions.items():
                mole_fractions[_species] = mole_fraction * correction_factor
            mole_fractions['0'] = mole_fraction_vacancy
        return mole_fractions


class SupercellXArgument(ArgumentBase):

    def __init__(self, options):
        super(SupercellXArgument, self).__init__(options, key='supercellx')

    def parse(self, options, *args, **kwargs):
        try:
            supercell_x = int(parse_float(self.raw_value, raise_exc=True))
        except ValueError:
            self.write_message('Could not parse supercell scale factor')
            raise InvalidOption
        else:
            return supercell_x


class SupercellYArgument(ArgumentBase):

    def __init__(self, options):
        super(SupercellYArgument, self).__init__(options, key='supercelly')

    def parse(self, options, *args, **kwargs):
        try:
            supercell_y = int(parse_float(self.raw_value, raise_exc=True))
        except ValueError:
            self.write_message('Could not parse supercell scale factor')
            raise InvalidOption
        else:
            return supercell_y


class SupercellZArgument(ArgumentBase):

    def __init__(self, options):
        super(SupercellZArgument, self).__init__(options, key='supercellz')

    def parse(self, options, *args, **kwargs):
        try:
            supercell_z = int(parse_float(self.raw_value, raise_exc=True))
        except ValueError:
            self.write_message('Could not parse supercell scale factor')
            raise InvalidOption
        else:
            return supercell_z


class SublatticeOption(ArgumentBase):

    def __init__(self, options):
        super(SublatticeOption, self).__init__(options, key='sublattice', option=True)

    def parse(self, options, *args, **kwargs):
        from pymatgen.core.periodic_table import Element
        calculation_structure = StructureFileArgument(options)()
        if not self.raw_value:
            return [calculation_structure]

        converters = [str, int, int, int, str]
        sublattice_structures = []
        for arg_string in self.raw_value:
            # Check list length and type conversion
            crumbs = [c for c in arg_string.split(',') if c != '']
            if len(crumbs) != len(converters):
                write_message(
                    'Invalid Argument: Sublattice specification must contains {0} arguments'.format(len(converters)))
                raise InvalidOption
            try:
                crumbs = [c(cr) for c, cr in zip(converters, crumbs)]
                structure_file_name, sx, sy, sz, species = crumbs
            except ValueError:
                write_message(
                    'Invalid argument type in --sublattice option. Format is <STRUCTURE>,<SUPERCELLX>,<SUPERCELLY>,<SUPERCELLZ>,<SPECIES>')
                raise InvalidOption

            species_list = [s for s in species.split(':') if s != '']
            # Check if given elements are valid chmical symbols
            for specie in species_list:
                try:
                    Element(specie)
                except ValueError:
                    write_message('"{0}" is not a valid specifier for a chemical element.'.format(species))
                    raise InvalidOption

            # Load structure to compare
            selection_structure = StructureFileArgument({'<structure>': structure_file_name})()
            selection_species = list(set([s.specie.symbol for s in selection_structure.sites]))

            # Check if species are present
            if not all([s in selection_species for s in species_list]):
                write_message(
                    'One of the sublattice species ("{0}") is not present in the original structure file ("{1}")!'.format(
                        species_list, selection_species))
                raise InvalidOption

            # Make selection supercell
            selection_structure.make_supercell([sx, sy, sz])

            if len(calculation_structure) != len(selection_structure):
                write_message(
                    'The number of atoms in the supercell of the structure file to select the sublattice does not match those the input structure file.')
                raise InvalidOption

            # Check if lattices match
            props = [
                lambda s: s.lattice.a,
                lambda s: s.lattice.b,
                lambda s: s.lattice.c,
                lambda s: s.lattice.alpha,
                lambda s: s.lattice.beta,
                lambda s: s.lattice.gamma
            ]
            if not all([isclose(p(selection_structure), p(calculation_structure), abs_tol=1e-3) for p in props]):
                write_message('Lattices do not match!')
                raise InvalidOption

            # Check if also sites match
            for calculation_site in calculation_structure:
                match_found = False
                for selection_site in selection_structure:
                    if isclose(calculation_site.distance(selection_site), 0.0, abs_tol=1e-3):
                        match_found = True
                        break
                if not match_found:
                    write_message('The structures do not match')
                    raise InvalidOption

            # Filtering
            selection_sites = [s for s in selection_structure.sites if s.specie.symbol in species_list]
            calculation_sites = []
            for selection_site in selection_sites:
                for calculation_site in calculation_structure:
                    if isclose(selection_site.distance(calculation_site), 0.0, abs_tol=1e-3):
                        calculation_sites.append(calculation_site)
                        break
            assert len(selection_sites) == len(calculation_sites)

            sublattice_structures.append(
                Structure(
                    calculation_structure.lattice,
                    [s.specie.symbol for s in calculation_sites],
                    [s.frac_coords for s in calculation_sites]
                )
            )
        return sublattice_structures
