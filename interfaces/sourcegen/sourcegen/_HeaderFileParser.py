"""Parser for header YAML configurations or existing CLib headers."""

# This file is part of Cantera. See License.txt in the top-level directory or
# at https://cantera.org/license.txt for license and copyright information.

from pathlib import Path
import logging
import re
from typing import List

from ._dataclasses import HeaderFile, Func, Recipe
from ._helpers import read_config


_logger = logging.getLogger()

_clib_path = Path(__file__).parent.joinpath("../../../include/cantera/clib").resolve()
_clib_ignore = ["clib_defs.h", "ctmatlab.h"]
_data_path = Path(__file__).parent.joinpath("_data").resolve()

class HeaderFileParser:

    def __init__(self, path: Path, ignore_funcs: List[str] = None):
        self._path = path
        self._ignore_funcs = ignore_funcs

    @classmethod
    def parse_yaml(cls, ignore_files, ignore_funcs) -> List[HeaderFile]:
        """Parse header file YAML configuration."""
        files = [_ for _ in _data_path.glob("*.yaml") if _.name not in ignore_files]
        return [cls(_, ignore_funcs.get(_.name, []))._parse_yaml() for _ in files]

    def _parse_yaml(self) -> HeaderFile:
        config = read_config(self._path)
        recipes = []
        cabinet = config.get("cabinet", [])
        prefix = cabinet["prefix"]
        base = cabinet["base"]
        parents = cabinet.get("parents", [])
        derived = cabinet.get("derived", [])
        uses = cabinet.get("uses", [])
        for func in cabinet["functions"]:
            func_name = f"{prefix}_{func['name']}"
            if func_name in self._ignore_funcs:
                continue
            recipes.append(
                Recipe(prefix,
                       func_name,
                       base,
                       parents,
                       derived,
                       uses,
                       func.get("implements", ""),
                       func.get("relates", ""),
                       func.get("what", "")))
        return HeaderFile(self._path, [], recipes)

    @classmethod
    def parse_h(cls, ignore_files, ignore_funcs) -> List[HeaderFile]:
        """Parse existing header file."""
        files = [_ for _ in _clib_path.glob("*.h")
                 if _.name not in ignore_files + _clib_ignore]
        return [cls(_, ignore_funcs.get(_.name, []))._parse_h() for _ in files]

    def _parse_h(self) -> HeaderFile:
        ct = self._path.read_text()

        matches = re.finditer(r"CANTERA_CAPI.*?;", ct, re.DOTALL)
        c_functions = [re.sub(r"\s+", " ", m.group()).replace("CANTERA_CAPI ", "")
                       for m in matches]

        if not c_functions:
            return

        parsed = map(Func.from_str, c_functions)

        _logger.info(f"  parsing {self._path.name}")
        if self._ignore_funcs:
            _logger.info(f"    ignoring {self._ignore_funcs}")

        parsed = [f for f in parsed if f.name not in self._ignore_funcs]

        if not parsed:
            return

        return HeaderFile(self._path, parsed)
