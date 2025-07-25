classdef ctTestInvalidInputs < ctTestCase

    properties
        phase
    end

    properties (SetAccess = protected)
        inputfile = '../data/invalid-inputs.yaml';
    end

    methods (Test)

        function testFailingConvert1(self)
            try
                self.phase = Solution(self.inputfile, 'A');
            catch ME
                self.verifySubstring(ME.identifier, 'Cantera:ctError');
                self.verifySubstring(ME.message, 'UnitSystem::convert');
            end
        end

        function testFailingConvert2(self)
            try
                self.phase = Solution(self.inputfile, 'B');
            catch ME
                self.verifySubstring(ME.identifier, 'Cantera:ctError');
                self.verifySubstring(ME.message, 'UnitSystem::convertActivationEnergy');
            end
        end

        function testFailingUnconfigured1(self)
            try
                self.phase = Solution(self.inputfile, 'C');
            catch ME
                self.verifySubstring(ME.identifier, 'Cantera:ctError');
                self.verifySubstring(ME.message, 'ArrheniusBase::validate');
            end
        end

        function testFailingUnconfigured2(self)
            try
                self.phase = Solution(self.inputfile, 'D');
            catch ME
                self.verifySubstring(ME.identifier, 'Cantera:ctError');
                self.verifySubstring(ME.message, 'FalloffRate::validate');
            end
        end

        function testFailingUnconfigured3(self)
            try
                self.phase = Solution(self.inputfile, 'E');
            catch ME
                self.verifySubstring(ME.identifier, 'Cantera:ctError');
                self.verifySubstring(ME.message, 'PlogRate::validate');
            end
        end

        function testFailingInvalidDuplicate(self)
            try
                self.phase = Solution(self.inputfile, 'F');
            catch ME
                self.verifySubstring(ME.identifier, 'Cantera:ctError');
                self.verifySubstring(ME.message, 'ArrheniusBase::check');
                self.verifySubstring(ME.message, 'negative pre-exponential factor');
            end
        end

    end
end
