classdef ctTestKineticsRepeatability < ctTestCase

    properties
        phase
        X0
        X1
    end

    properties (SetAccess = protected)
        T0 = 1200;
        T1 = 1300;
        rho0 = 2.4;
        rho1 = 3.1;
        P0 = 1.4e+05;
        P1 = 3.7e+06;
        rtol = 1e-6;
        atol = 1e-8;
    end

    methods

        function setup_phase(self, mech)
            self.phase = Solution(mech);
            self.X0 = 1 + sin(1:self.phase.nSpecies);
            self.X1 = 1 + sin(2:self.phase.nSpecies + 1);
        end

        function checkRatesX(self, mech)
            self.setup_phase(mech);

            self.phase.TDX = {self.T0, self.rho0, self.X0};
            w1 = self.phase.netProdRates;

            self.phase.TDX = {self.T1, self.rho1, self.X1};
            w2 = self.phase.netProdRates;

            self.phase.TDX = {self.T0, self.rho0, self.X1};
            w3 = self.phase.netProdRates;

            self.phase.TDX = {self.T0, self.rho0, self.X0};
            w4 = self.phase.netProdRates;

            self.verifyEqual(w1, w4, 'RelTol', self.rtol);
        end

        function checkRatesT1(self, mech)
            self.setup_phase(mech);

            self.phase.TDX = {self.T0, self.rho0, self.X0};
            w1 = self.phase.netProdRates;

            self.phase.TDX = {self.T1, self.rho1, self.X1};
            w2 = self.phase.netProdRates;

            self.phase.TDX = {self.T1, self.rho0, self.X0};
            w3 = self.phase.netProdRates;

            self.phase.TDX = {self.T0, self.rho0, self.X0};
            w4 = self.phase.netProdRates;

            self.verifyEqual(w1, w4, 'RelTol', self.rtol);
        end

        function checkRatesT2(self, mech)
            self.setup_phase(mech);

            self.phase.TPX = {self.T0, self.P0, self.X0};
            w1 = self.phase.netProdRates;

            self.phase.TPX = {self.T1, self.P1, self.X1};
            w2 = self.phase.netProdRates;

            self.phase.TPX = {self.T1, self.P0, self.X0};
            w3 = self.phase.netProdRates;

            self.phase.TPX = {self.T0, self.P0, self.X0};
            w4 = self.phase.netProdRates;

            self.verifyEqual(w1, w4, 'RelTol', self.rtol);
        end

        function checkRatesP(self, mech)
            self.setup_phase(mech);

            self.phase.TPX = {self.T0, self.P0, self.X0};
            w1 = self.phase.netProdRates;

            self.phase.TPX = {self.T1, self.P1, self.X1};
            w2 = self.phase.netProdRates;

            self.phase.TPX = {self.T0, self.P1, self.X0};
            w3 = self.phase.netProdRates;

            self.phase.TPX = {self.T0, self.P0, self.X0};
            w4 = self.phase.netProdRates;

            self.verifyEqual(w1, w4, 'RelTol', self.rtol);
        end

    end

    methods (Test)

        function testGRI30X(self)
            self.checkRatesX('gri30.yaml');
        end

        function testGRI30T(self)
            self.checkRatesT1('gri30.yaml');
            self.checkRatesT2('gri30.yaml');
        end

        function testGRI30P(self)
            self.checkRatesP('gri30.yaml');
        end

        function testH2O2X(self)
            self.checkRatesX('h2o2.yaml');
        end

        function testH2O2T(self)
            self.checkRatesT1('h2o2.yaml');
            self.checkRatesT2('h2o2.yaml');
        end

        function testH2O2P(self)
            self.checkRatesP('h2o2.yaml');
        end

        function testPdepX(self)
            self.checkRatesX('../data/pdep-test.yaml');
        end

        function testPdepT(self)
            self.checkRatesT1('../data/pdep-test.yaml');
            self.checkRatesT2('../data/pdep-test.yaml');
        end

        function testPdepP(self)
            self.checkRatesP('../data/pdep-test.yaml');
        end

    end

end
