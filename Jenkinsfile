#!groovy

pipeline {
  agent {
    docker {
      image 'gassmoeller/aspect-tester:8.5.0'
    }
  }

  options {
    timeout(time: 2, unit: 'HOURS')
  }

  stages {
    stage('Check Indentation') {
      steps {
        sh './doc/indent'
        sh 'git diff > changes-astyle.diff'
        archiveArtifacts artifacts: 'changes-astyle.diff', fingerprint: true
        sh '''
          git diff --exit-code || \
          { echo "Please check indentation, see artifacts in the top right corner!"; exit 1; }
        '''
      }
    }

    stage('Build') {
      options {
        timeout(time: 15, unit: 'MINUTES')
      }
      steps {
        sh 'mkdir build-gcc-fast'

        sh '''
          cd build-gcc-fast
          cmake \
            -G "Ninja" \
            -D CMAKE_CXX_FLAGS='-Werror' \
            -D ASPECT_TEST_GENERATOR='Ninja' \
            -D ASPECT_USE_PETSC='OFF' \
            -D ASPECT_RUN_ALL_TESTS='ON' \
            -D ASPECT_PRECOMPILE_HEADERS='ON' \
            "${WORKSPACE}"
          '''

        sh '''
          cd build-gcc-fast
          ninja
        '''
      }
    }

    stage('Test') {
      options {
        timeout(time: 90, unit: 'MINUTES')
      }
      steps {
        sh '''
          # This export avoids a warning about
          # a discovered, but unconnected infiniband network.
          export OMPI_MCA_btl=self,tcp

          cd build-gcc-fast/tests

          # Let ninja prebuild the test libraries and run
          # the tests to create the output files in parallel. We
          # want this to always succeed, because it does not generate
          # useful output (we do this further down using 'ctest', however
          # ctest can not run ninja in parallel, so this is the
          # most efficient way to build the tests).
          ninja -k 0 tests || true
        '''

        sh '''
          # Avoid the warning described above
          export OMPI_MCA_btl=self,tcp

          cd build-gcc-fast

          # Output the test results using ctest. Since
          # the tests were prebuild in the previous shell
          # command, this will be fast although it is not
          # running in parallel.
          ctest \
            --no-compress-output \
            --test-action Test
        '''
      }

      post {
        always {
          // Generate the 'Tests' output page in Jenkins
          xunit testTimeMargin: '3000',
            thresholdMode: 1,
            thresholds: [failed(), skipped()],
            tools: [CTest(pattern: 'build-gcc-fast/Testing/**/*.xml')]

          // Update the reference test output with the new test results
          sh '''
            export OMPI_MCA_btl=self,tcp
            cd build-gcc-fast
            ninja generate_reference_output
          '''

          // Revert the change to the mpirun command we made above, so
          // that the modification does not show up in the 'git diff' command
          sh 'git checkout tests/CMakeLists.txt'

          // Generate the 'Artifacts' diff-file that can be
          // used to update the test results
          sh 'git diff tests > changes-test-results.diff'
          archiveArtifacts artifacts: 'changes-tests-results.diff', fingerprint: true
        }
      }
    }
  }
}
